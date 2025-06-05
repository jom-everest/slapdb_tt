/*
Каждое событие содержит:
- время (64 битное целое число (timestamp (unsigned long) в миллисекундах от начала EPOCH))
- тип события:
enum EventType {
AttachRequest, // ENODEB --> MME
IdentityResponse, // ENODEB --> MME
AttachAccept, // MME --> ENODEB
Paging, // ENODEB --> MME
PathSwitchRequest, // ENODEB --> MME
PathSwitchRequestAcknowledge, // MME --> ENODEB
UEContextReleaseCommand, // ENODEB --> MME
UEContextReleaseResponse, // MME --> ENODEB
};
- enodeb_id (32 битное беззнаковое целое: unsigned int) в сообщениях: во всех кроме Paging
- mme_id (32 битное беззнаковое целое: unsigned int) в сообщениях: во всех кроме
AttachRequest, Paging
- imsi (64 битное беззнаковое целое: unsigned long) в сообщениях: AttachRequest
(опционально, но что-то одно будет или imsi или m_tmsi), IdentityResponse
- m_tmsi (32 битное беззнаковое целое: unsigned int) в сообщениях:
AttachRequest(опционально), AttachAccept, Paging
- cgi (std::vector<unsigned char>) может содержаться только в сообщениях типа: ENODEB -->
MME
*/

#pragma once

#include <cstdint>
#include <vector>

enum class SlapEventType : uint8_t
{
    AttachRequest, // ENODEB --> MME
    IdentityResponse, // ENODEB --> MME
    AttachAccept, // MME --> ENODEB
    Paging, // ENODEB --> MME
    PathSwitchRequest, // ENODEB --> MME
    PathSwitchRequestAcknowledge, // MME --> ENODEB
    UEContextReleaseCommand, // ENODEB --> MME
    UEContextReleaseResponse, // MME --> ENODEB
};

struct SlapEvent 
{
    uint64_t timestamp; // - время (64 битное целое число (timestamp (unsigned long) в миллисекундах от начала EPOCH))
    SlapEventType event_type; // - тип события:
    uint32_t enodeb_id; // (32 битное беззнаковое целое: unsigned int) в сообщениях: во всех кроме Paging
    uint32_t mme_id; // (32 битное беззнаковое целое: unsigned int) в сообщениях: во всех кроме AttachRequest, Paging
    uint64_t imsi; // (64 битное беззнаковое целое: unsigned long) в сообщениях: AttachRequest (опционально, но что-то одно будет или imsi или m_tmsi), IdentityResponse
    uint32_t m_tmsi; // (32 битное беззнаковое целое: unsigned int) в сообщениях: AttachRequest(опционально), AttachAccept, Paging
    std::vector<uint8_t> cgi; // (std::vector<unsigned char>) может содержаться только в сообщениях типа: ENODEB --> MME
};

