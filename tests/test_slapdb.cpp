
#include <gtest/gtest.h>
#include "slapdb.h"

TEST(SlapDbTest, Attach_1)
{
    SlapDb db;
    SlapEvent ev;

    ev.event_type = SlapEventType::AttachRequest;
    ev.timestamp = 1;
    ev.cgi = {1, 1};
    ev.enodeb_id = 1;
    ev.imsi = 11;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachAccept;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.m_tmsi = 111;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::Reg, 11, {1,1}));
}

TEST(SlapDbTest, Attach_2)
{
    SlapDb db;
    SlapEvent ev;

    ev.event_type = SlapEventType::AttachRequest;
    ev.timestamp = 1;
    ev.cgi = {1, 1};
    ev.enodeb_id = 1;
    ev.imsi = 11;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachRequest;
    ev.enodeb_id = 1;
    ev.imsi = 12;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachAccept;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.m_tmsi = 111;
    EXPECT_EQ(db.handler(ev), std::nullopt);
}

TEST(SlapDbTest, Attach_3)
{
    SlapDb db;
    SlapEvent ev;

    ev.event_type = SlapEventType::AttachRequest;
    ev.timestamp = 1;
    ev.cgi = {1, 1};
    ev.enodeb_id = 1;
    ev.imsi = 11;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachRequest;
    ev.enodeb_id = 2;
    ev.imsi = 12;
    ev.cgi = {1, 2};
    db.handler(ev);

    ev.event_type = SlapEventType::AttachAccept;
    ev.enodeb_id = 2;
    ev.mme_id = 1;
    ev.m_tmsi = 112;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::Reg, 12, {1, 2}));
}

TEST(SlapDbTest, AttachWithIdentityResponse_1)
{
    SlapDb db;
    SlapEvent ev;

    ev.event_type = SlapEventType::AttachRequest;
    ev.timestamp = 1;
    ev.cgi = {1, 1};
    ev.enodeb_id = 1;
    ev.imsi = 11;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachRequest;
    ev.enodeb_id = 1;
    ev.imsi = 12;
    ev.cgi = {1, 2};
    db.handler(ev);

    ev.event_type = SlapEventType::IdentityResponse;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.imsi = 11;
    ev.cgi = {3, 3};
    db.handler(ev);

    ev.event_type = SlapEventType::AttachAccept;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.m_tmsi = 112;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::Reg, 11, {3, 3}));
}

TEST(SlapDbTest, AttachWithTimeout_1)
{
    SlapDb db;
    SlapEvent ev;

    ev.event_type = SlapEventType::AttachRequest;
    ev.timestamp = 1;
    ev.cgi = {1, 1};
    ev.enodeb_id = 1;
    ev.imsi = 11;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachAccept;
    ev.timestamp = 1002;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.m_tmsi = 111;
    EXPECT_EQ(db.handler(ev), std::nullopt);
}


TEST(SlapDbTest, Paging_1)
{
    SlapDb db;
    SlapEvent ev;

    ev.event_type = SlapEventType::AttachRequest;
    ev.timestamp = 1;
    ev.cgi = {1, 1};
    ev.enodeb_id = 1;
    ev.imsi = 11;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachAccept;
    ev.timestamp = 1;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.m_tmsi = 111;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::Reg, 11, {1, 1}));

    ev.event_type = SlapEventType::Paging;
    ev.timestamp = 2;
    ev.m_tmsi = 111;
    ev.cgi = {5, 5};
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::Cgi, 11, {5, 5}));

    ev.event_type = SlapEventType::Paging;
    ev.timestamp = 2;
    ev.m_tmsi = 111;
    ev.cgi = {5, 6};
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::Cgi, 11, {5, 6}));
}

TEST(SlapDbTest, CallFlow_1)
{
    SlapDb db;
    SlapEvent ev;

    ev.event_type = SlapEventType::AttachRequest;
    ev.timestamp = 1;
    ev.cgi = {1, 1};
    ev.enodeb_id = 1;
    ev.imsi = 11;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachAccept;
    ev.timestamp = 1;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.m_tmsi = 111;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::Reg, 11, {1, 1}));

    ev.event_type = SlapEventType::PathSwitchRequest;
    ev.enodeb_id = 2;
    ev.mme_id = 1;
    ev.cgi = {2, 2};
    db.handler(ev);

    ev.event_type = SlapEventType::PathSwitchRequestAcknowledge;
    ev.enodeb_id = 2;
    ev.mme_id = 2;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::Cgi, 11, {2, 2}));

    ev.event_type = SlapEventType::PathSwitchRequest;
    ev.enodeb_id = 3;
    ev.mme_id = 2;
    ev.cgi = {3, 3};
    db.handler(ev);

    ev.event_type = SlapEventType::PathSwitchRequestAcknowledge;
    ev.enodeb_id = 3;
    ev.mme_id = 3;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::Cgi, 11, {3, 3}));

    ev.event_type = SlapEventType::UEContextReleaseCommand;
    ev.enodeb_id = 3;
    ev.mme_id = 3;
    ev.cgi = {7,8};
    db.handler(ev);

    ev.event_type = SlapEventType::UEContextReleaseResponse;
    ev.enodeb_id = 3;
    ev.mme_id = 3;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::UnReg, 11, {7, 8}));
}


TEST(SlapDbTest, CallFlow_2)
{
    SlapDb db;
    SlapEvent ev;

    ev.event_type = SlapEventType::AttachRequest;
    ev.timestamp = 1;
    ev.cgi = {1, 1};
    ev.enodeb_id = 1;
    ev.imsi = 11;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachAccept;
    ev.timestamp = 1;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.m_tmsi = 111;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::Reg, 11, {1, 1}));

    ev.event_type = SlapEventType::PathSwitchRequest;
    ev.enodeb_id = 2;
    ev.mme_id = 1;
    ev.cgi = {2, 2};
    db.handler(ev);

// timeout
    ev.event_type = SlapEventType::PathSwitchRequestAcknowledge;
    ev.timestamp = 1003;
    ev.enodeb_id = 2;
    ev.mme_id = 2;
    EXPECT_EQ(db.handler(ev), std::nullopt);

    ev.event_type = SlapEventType::PathSwitchRequest;
    ev.enodeb_id = 3;
    ev.mme_id = 1;
    ev.cgi = {3, 3};
    db.handler(ev);

    ev.event_type = SlapEventType::PathSwitchRequestAcknowledge;
    ev.enodeb_id = 3;
    ev.mme_id = 3;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::Cgi, 11, {3, 3}));

    ev.event_type = SlapEventType::UEContextReleaseCommand;
    ev.enodeb_id = 3;
    ev.mme_id = 3;
    ev.cgi = {7,8};
    db.handler(ev);

    ev.event_type = SlapEventType::UEContextReleaseResponse;
    ev.enodeb_id = 3;
    ev.mme_id = 3;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::UnReg, 11, {7, 8}));
}


TEST(SlapDbTest, CallFlow_3)
{
    SlapDb db;
    SlapEvent ev;

    ev.event_type = SlapEventType::AttachRequest;
    ev.timestamp = 1;
    ev.cgi = {1, 1};
    ev.enodeb_id = 1;
    ev.imsi = 11;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachAccept;
    ev.timestamp = 1;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.m_tmsi = 111;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::Reg, 11, {1, 1}));

    ev.event_type = SlapEventType::UEContextReleaseCommand;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.cgi = {7,8};
    db.handler(ev);

    ev.event_type = SlapEventType::UEContextReleaseResponse;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::UnReg, 11, {7, 8}));

// -----------------    NEW CALL FLOW
    ev.event_type = SlapEventType::AttachRequest;
    ev.timestamp = 10;
    ev.cgi = {1, 1};
    ev.enodeb_id = 4;
    ev.m_tmsi = 111;
    ev.imsi = 0;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachAccept;
    ev.enodeb_id = 4;
    ev.mme_id = 4;
    ev.m_tmsi = 111;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::Reg, 11, {1, 1}));
}

TEST(SlapDbTest, CallFlowTiteout24hours_1)
{
    SlapDb db;
    SlapEvent ev;

    ev.event_type = SlapEventType::AttachRequest;
    ev.timestamp = 1;
    ev.cgi = {1, 1};
    ev.enodeb_id = 1;
    ev.imsi = 11;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachAccept;
    ev.timestamp = 1;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.m_tmsi = 111;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::Reg, 11, {1, 1}));

    ev.event_type = SlapEventType::UEContextReleaseCommand;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.cgi = {7,8};
    db.handler(ev);

    ev.event_type = SlapEventType::UEContextReleaseResponse;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::UnReg, 11, {7, 8}));

    // -----------------    NEW CALL FLOW
    ev.event_type = SlapEventType::AttachRequest;
    ev.timestamp = 10 + 24*60*60*1000;
    ev.cgi = {1, 1};
    ev.enodeb_id = 4;
    ev.m_tmsi = 111;
    ev.imsi = 0;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachAccept;
    ev.enodeb_id = 4;
    ev.mme_id = 4;
    ev.m_tmsi = 111;
    EXPECT_EQ(db.handler(ev), std::nullopt);
}

TEST(SlapDbTest, CallFlowTimeout24hours_2)
{
    SlapDb db;
    SlapEvent ev;

    ev.event_type = SlapEventType::AttachRequest;
    ev.timestamp = 1;
    ev.cgi = {1, 1};
    ev.enodeb_id = 1;
    ev.imsi = 11;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachAccept;
    ev.timestamp = 1;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.m_tmsi = 111;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::Reg, 11, {1, 1}));

    ev.event_type = SlapEventType::UEContextReleaseCommand;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.cgi = {7,8};
    db.handler(ev);

    ev.event_type = SlapEventType::UEContextReleaseResponse;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    EXPECT_EQ(db.handler(ev).value(), SlapOut(SlapOut::SlapOutType::UnReg, 11, {7, 8}));

    // -----------------    NEW CALL FLOW
    ev.event_type = SlapEventType::AttachRequest;
    ev.timestamp = 10 + 24*60*60*1000;
    ev.cgi = {1, 1};
    ev.enodeb_id = 4;
    ev.m_tmsi = 111;
    ev.imsi = 0;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachAccept;
    ev.enodeb_id = 4;
    ev.mme_id = 4;
    ev.m_tmsi = 111;
    EXPECT_EQ(db.handler(ev), std::nullopt);
}

TEST(SlapDbTest, CallFlowInvalid_1)
{
    SlapDb db;
    SlapEvent ev;

    ev.event_type = SlapEventType::AttachRequest;
    ev.timestamp = 1;
    ev.cgi = {1, 1};
    ev.enodeb_id = 1;
    ev.imsi = 11;
    db.handler(ev);

    ev.event_type = SlapEventType::AttachAccept;
    ev.timestamp = 1002;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.m_tmsi = 111;
    EXPECT_EQ(db.handler(ev), std::nullopt);

    ev.event_type = SlapEventType::PathSwitchRequestAcknowledge;
    ev.enodeb_id = 3;
    ev.mme_id = 3;
    EXPECT_EQ(db.handler(ev), std::nullopt);

    ev.event_type = SlapEventType::UEContextReleaseCommand;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    ev.cgi = {7,8};
    db.handler(ev);

    ev.event_type = SlapEventType::UEContextReleaseResponse;
    ev.enodeb_id = 1;
    ev.mme_id = 1;
    EXPECT_EQ(db.handler(ev), std::nullopt);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


