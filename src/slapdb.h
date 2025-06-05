
#pragma once

#include <optional>
#include <unordered_map>

#include "slap_event.h"

struct SlapOut 
{
    enum class SlapOutType : uint8_t  { Reg, UnReg, Cgi};

    SlapOut(SlapOutType type, uint64_t imsi, const std::vector<uint8_t>& cgi) : 
        slap_type(type),
        imsi(imsi),
        cgi(cgi)
    {}

    bool operator==(const SlapOut& other) const {
        return slap_type == other.slap_type &&
               imsi == other.imsi &&
               cgi == other.cgi;
    }

    SlapOutType slap_type;
    uint64_t imsi;
    std::vector<uint8_t> cgi;
};

class SlapDb 
{
    struct ImsiRecord
    {
        enum class State : uint8_t {AttachRequest, AttachFinished, Paging, PathSwitchRequest};
        State state;
        uint64_t last_event_timestamp;
        uint64_t attach_timestamp;
        uint64_t init_timestamp;
        uint32_t enodeb_id;
        uint32_t mme_id;
        uint32_t m_tmsi;
        std::vector<uint8_t> cgi;
    };

    struct PathSwitchRecord
    {
        uint64_t timestamp;
        uint32_t old_mme_id;
        uint32_t old_enodeb_id;
        uint32_t new_enodeb_id;
        std::vector<uint8_t> cgi;
    };

    struct ReleaseRecord
    {
        uint64_t timestamp;
        uint32_t mme_id;
        uint32_t enodeb_id;
        std::vector<uint8_t> cgi;
    };

public:
    std::optional<SlapOut> handler(const SlapEvent& event);

private:
    std::optional<SlapOut> attach_request_handler_1(const SlapEvent& event);
    std::optional<SlapOut> identity_response_handler_2(const SlapEvent& event);
    std::optional<SlapOut> attach_accept_handler_3(const SlapEvent& event);
    std::optional<SlapOut> paging_handler_4(const SlapEvent& event);
    std::optional<SlapOut> path_switch_request_handler_5(const SlapEvent& event);
    std::optional<SlapOut> path_switch_request_acknowledge_handler_6(const SlapEvent& event);
    std::optional<SlapOut> uecontext_release_command_handler_7(const SlapEvent& event);
    std::optional<SlapOut> uecontext_release_response_handler_8(const SlapEvent& event);

private:
    std::unordered_map<uint64_t, ImsiRecord> _imsi_list;
    std::unordered_multimap<uint32_t, uint64_t> _enodeb_to_imsi_attach_tmp;
    std::unordered_map<uint32_t, uint64_t> _tmsi_to_imsi;
    std::unordered_multimap<uint32_t, uint64_t> _mme_to_imsi;
    std::unordered_map<uint64_t, PathSwitchRecord>_pathswitch_list_tmp;
    std::unordered_map<uint64_t, ReleaseRecord>_release_list_tmp;
};

