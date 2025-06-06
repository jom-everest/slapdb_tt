
#include "slapdb.h"

const uint64_t TIMEOUT_SEC = 1000;
const uint64_t TIMEOUT_24HOURS = 24 * 60 * 60 * 1000;
const uint64_t TIMEOUT_CLEANUP = 60 * 60 * 1000;


std::optional<SlapOut> SlapDb::handler(const SlapEvent& event)
{
    switch (event.event_type)
    {
    case SlapEventType::AttachRequest:
        return attach_request_handler_1(event);

    case SlapEventType::IdentityResponse:
        return identity_response_handler_2(event);

    case SlapEventType::AttachAccept:
        return attach_accept_handler_3(event);

    case SlapEventType::Paging:
        return paging_handler_4(event);

    case SlapEventType::PathSwitchRequest:
        return path_switch_request_handler_5(event);

    case SlapEventType::PathSwitchRequestAcknowledge:
        return path_switch_request_acknowledge_handler_6(event);

    case SlapEventType::UEContextReleaseCommand:
        return uecontext_release_command_handler_7(event);

    case SlapEventType::UEContextReleaseResponse:
        return uecontext_release_response_handler_8(event);

    default:
        break;
    }

    return std::nullopt;
}


void SlapDb::cleanup_each_hour(uint64_t timestamp)
{
    if (timestamp - cleanup_timestamp < TIMEOUT_CLEANUP) return;

    cleanup_timestamp = timestamp;
    for (auto it = _imsi_list.begin(); it != _imsi_list.end(); ) {
        if (timestamp - it->second.init_timestamp > TIMEOUT_24HOURS) {
            uint64_t imsi = it->first;
            it = _imsi_list.erase(it);

            // удаление связанных данных из таблиц _tmsi_to_imsi, _mme_to_imsi
            erase_in_tmsi_to_imsi(imsi);
            erase_in_mme_to_imsi(imsi);
        }
        else {
            ++it;
        }
    }
}

void SlapDb::erase_in_tmsi_to_imsi(uint64_t imsi)
{
    for (auto it = _tmsi_to_imsi.begin(); it != _tmsi_to_imsi.end(); ) {
        if (it->second == imsi) {
            it = _tmsi_to_imsi.erase(it);
        }
        else {
            ++it;
        }
    }
}


void SlapDb::erase_in_mme_to_imsi(uint64_t imsi)
{
    for (auto it = _mme_to_imsi.begin(); it != _mme_to_imsi.end(); ) {
        if (it->second == imsi) {
            it = _mme_to_imsi.erase(it);
        }
        else {
            ++it;
        }
    }
}


std::optional<SlapOut> SlapDb::attach_request_handler_1(const SlapEvent& event)
{
    // запуск процеудры обнаружегния и удаления старых imsi каждые 1 час
    cleanup_each_hour(event.timestamp);

    if (event.imsi == 0 && event.m_tmsi) {
        auto it = _tmsi_to_imsi.find(event.m_tmsi);
        if (it == _tmsi_to_imsi.end()) return std::nullopt;

        auto& elem = _imsi_list[it->second];
        if (event.timestamp - elem.init_timestamp > TIMEOUT_24HOURS) {
            _tmsi_to_imsi.erase(it);
            return std::nullopt;
        }

        _enodeb_to_imsi_attach_tmp.emplace(event.enodeb_id, it->second);

        elem.state = ImsiRecord::State::AttachRequest;
        elem.attach_timestamp = elem.last_event_timestamp = event.timestamp;
        elem.enodeb_id = event.enodeb_id;
        elem.cgi = event.cgi;
    }
    else {
        // добавляем в индекс соотвествующую пару enodeb_id, imsi
        _enodeb_to_imsi_attach_tmp.emplace(event.enodeb_id, event.imsi);

        // обновлнеие таблицы абонентов, timesptamp, enodeb_id
        auto& elem = _imsi_list[event.imsi];
        elem.state = ImsiRecord::State::AttachRequest;
        elem.init_timestamp = elem.attach_timestamp = elem.last_event_timestamp = event.timestamp;
        elem.enodeb_id = event.enodeb_id;
        elem.cgi = event.cgi;
    }

    return std::nullopt;
}

// завершение подтверждение регистрации
std::optional<SlapOut> SlapDb::attach_accept_handler_3(const SlapEvent& event)
{
    // поиск соответствующего imsi по enodeb_id, mme_id

    // формируем список всех imsi для enodeb_id
    std::vector<std::pair<decltype(_enodeb_to_imsi_attach_tmp)::iterator, ImsiRecord&>> imsi_list;
    auto range = _enodeb_to_imsi_attach_tmp.equal_range(event.enodeb_id);
    for (auto it = range.first; it != range.second; ) {
        auto& elem = _imsi_list[it->second];
        if (event.timestamp - elem.attach_timestamp > TIMEOUT_SEC) {
            // удаление при таймауте
            it = _enodeb_to_imsi_attach_tmp.erase(it);
            continue;
        }
        if (elem.mme_id == 0 || elem.mme_id == event.mme_id) {
            imsi_list.push_back({it, elem});
        }
        it++;
    }

    // в этом списке поиск по mme
    std::optional<std::pair<decltype(_enodeb_to_imsi_attach_tmp)::iterator, ImsiRecord&>> ref_imsi = std::nullopt;
    for (auto it : imsi_list) {
        if (it.second.mme_id == event.mme_id) {
            if (ref_imsi) {
                // нашли более 1 imsi - выход
                return std::nullopt;
            }
            ref_imsi = it;
        }
    }

    // не нашли imsi по enodeb_id, mme_id - тогда проверка кол0во элементов
    if (!ref_imsi) {
        if (imsi_list.size() != 1) {
            return std::nullopt;
        }
        ref_imsi = imsi_list[0];
    }

    // нашли единственный imsi
    ref_imsi->second.last_event_timestamp = event.timestamp;
    ref_imsi->second.mme_id = event.mme_id;
    ref_imsi->second.m_tmsi = event.m_tmsi;
    ref_imsi->second.state = ImsiRecord::State::AttachFinished;
    auto imsi = ref_imsi->first->second;

    // добавление в таблицу соответсвия tmsi - imsi
    _tmsi_to_imsi.emplace(event.m_tmsi, imsi);

    // удаление из тадлицы ожидающих подтверждения
    _enodeb_to_imsi_attach_tmp.erase(ref_imsi->first);

    _mme_to_imsi.emplace(event.mme_id, imsi);

    return SlapOut(SlapOut::SlapOutType::Reg, imsi, ref_imsi->second.cgi);
}

std::optional<SlapOut> SlapDb::identity_response_handler_2(const SlapEvent& event)
{
    auto it = _imsi_list.find(event.imsi);
    if (it == _imsi_list.end()
        || it->second.enodeb_id != event.enodeb_id
        || it->second.state != ImsiRecord::State::AttachRequest
        || (event.timestamp - it->second.attach_timestamp) > TIMEOUT_SEC
    ) return std::nullopt;

    // обновление время последнего event, mme_id, cgi
    it->second.last_event_timestamp = event.timestamp;
    it->second.mme_id = event.mme_id;
    it->second.cgi = event.cgi;

    return SlapOut(SlapOut::SlapOutType::Cgi, event.imsi, event.cgi);
}


std::optional<SlapOut> SlapDb::paging_handler_4(const SlapEvent& event)
{
    auto it = _tmsi_to_imsi.find(event.m_tmsi);
    if (it == _tmsi_to_imsi.end()) return std::nullopt;

    auto& elem = _imsi_list[it->second];
    elem.cgi = event.cgi;
    elem.last_event_timestamp = event.timestamp;

    return SlapOut(SlapOut::SlapOutType::Cgi, it->second, elem.cgi);
}

std::optional<SlapOut> SlapDb::path_switch_request_handler_5(const SlapEvent& event)
{
    if (_mme_to_imsi.count(event.mme_id) != 1) return std::nullopt;

    auto imsi = _mme_to_imsi.equal_range(event.mme_id).first->second;
    auto& elem = _imsi_list[imsi];
    auto& elem_from_pathswitch = _pathswitch_list_tmp[imsi];

    elem_from_pathswitch.timestamp = event.timestamp;
    elem_from_pathswitch.cgi = event.cgi;
    elem_from_pathswitch.old_enodeb_id = elem.enodeb_id;
    elem_from_pathswitch.new_enodeb_id = event.enodeb_id;
    elem_from_pathswitch.old_mme_id = event.mme_id;

    elem.last_event_timestamp = event.timestamp;

    return std::nullopt;
}

std::optional<SlapOut> SlapDb::path_switch_request_acknowledge_handler_6(const SlapEvent& event)
{
    std::optional<decltype(_pathswitch_list_tmp)::iterator> it_pathswitch = std::nullopt;
    for (auto it = _pathswitch_list_tmp.begin(); it != _pathswitch_list_tmp.end();) {
        if (event.timestamp - it->second.timestamp > TIMEOUT_SEC) {
            it = _pathswitch_list_tmp.erase(it);
        }
        else {
            if (it->second.new_enodeb_id == event.enodeb_id) {
                if (it_pathswitch) return std::nullopt;
                it_pathswitch = it;
            }
            ++it;
        }
    }

    if (!it_pathswitch) return std::nullopt;

    auto imsi = it_pathswitch.value()->first;
    auto& elem = _imsi_list[imsi];
    auto& elem_from_pathswitch = it_pathswitch.value()->second;

    elem.enodeb_id = elem_from_pathswitch.new_enodeb_id;
    elem.mme_id = event.mme_id;
    elem.last_event_timestamp = event.timestamp;
    elem.cgi = elem_from_pathswitch.cgi;

    auto range = _mme_to_imsi.equal_range(elem_from_pathswitch.old_mme_id);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == imsi) {
            auto node = _mme_to_imsi.extract(it);
            node.key() = event.mme_id;
            _mme_to_imsi.insert(std::move(node));
            break;
        }
    }

    _pathswitch_list_tmp.erase(it_pathswitch.value());

    return SlapOut(SlapOut::SlapOutType::Cgi, imsi, elem.cgi);
}


std::optional<SlapOut> SlapDb::uecontext_release_command_handler_7(const SlapEvent& event)
{
    std::optional<decltype(_imsi_list)::iterator> it_db = std::nullopt;
    auto range = _mme_to_imsi.equal_range(event.mme_id);
    for (auto it = range.first; it != range.second; ++it) {
        auto it_db_tmp = _imsi_list.find(it->second);
        if (it_db_tmp->second.enodeb_id == it_db_tmp->second.enodeb_id) {
            if (it_db) {
                return std::nullopt;
            }
            it_db = it_db_tmp;
        }
    }

    if (it_db) {
        auto imsi = it_db.value()->first;
        auto& elem = it_db.value()->second;
        auto& elem_from_release = _release_list_tmp[imsi];

        elem_from_release.timestamp = event.timestamp;
        elem_from_release.cgi = event.cgi;
        elem_from_release.enodeb_id = elem.enodeb_id;
        elem_from_release.mme_id = event.mme_id;

        elem.last_event_timestamp = event.timestamp;
    }

    return std::nullopt;
}


std::optional<SlapOut> SlapDb::uecontext_release_response_handler_8(const SlapEvent& event)
{
    auto it_release = search_in_release_table(event.timestamp, event.enodeb_id, event.mme_id);
    if (!it_release) return std::nullopt;

    auto imsi = it_release.value()->first;
    auto& elem = _imsi_list[imsi];
    elem.last_event_timestamp = event.timestamp;
    elem.cgi = it_release.value()->second.cgi;

    auto range = _mme_to_imsi.equal_range(elem.mme_id);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == imsi) {
            _mme_to_imsi.erase(it);
            break;
        }
    }

    _release_list_tmp.erase(it_release.value());

    elem.mme_id = elem.enodeb_id = 0;

    return SlapOut(SlapOut::SlapOutType::UnReg, imsi, elem.cgi);
}


std::optional<std::unordered_map<uint64_t, SlapDb::ReleaseRecord>::iterator>
SlapDb::search_in_release_table(uint64_t timestamp, uint32_t enodeb_id, uint32_t mme_id)
{
    std::optional<decltype(_release_list_tmp)::iterator> it_release = std::nullopt;
    for (auto it = _release_list_tmp.begin(); it != _release_list_tmp.end();) {
        if (timestamp - it->second.timestamp > TIMEOUT_SEC) {
            it = _release_list_tmp.erase(it);
        }
        else {
            if (it->second.enodeb_id == enodeb_id && it->second.mme_id == mme_id) {
                if (it_release) return std::nullopt;
                it_release = it;
            }
            ++it;
        }
    }

    return it_release;
}

