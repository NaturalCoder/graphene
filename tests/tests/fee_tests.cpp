/*
 * Copyright (c) 2015, Cryptonomex, Inc.
 * All rights reserved.
 *
 * This source code is provided for evaluation in private test networks only, until September 8, 2015. After this date, this license expires and
 * the code may not be used, modified or distributed for any purpose. Redistribution and use in source and binary forms, with or without modification,
 * are permitted until September 8, 2015, provided that the following conditions are met:
 *
 * 1. The code and/or derivative works are used only for private test networks consisting of no more than 10 P2P nodes.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <graphene/chain/vesting_balance_object.hpp>

#include <boost/test/unit_test.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;

BOOST_FIXTURE_TEST_SUITE( fee_tests, database_fixture )

BOOST_AUTO_TEST_CASE( cashback_test )
{ try {
   /*                        Account Structure used in this test                         *
    *                                                                                    *
    *               /-----------------\       /-------------------\                      *
    *               | life (Lifetime) |       | reggie (Lifetime) |                      *
    *               \-----------------/       \-------------------/                      *
    *                  | Refers &   | Refers     | Registers  | Registers                *
    *                  v Registers  v            v            |                          *
    *  /----------------\         /----------------\          |                          *
    *  |  ann (Annual)  |         |  dumy (basic)  |          |                          *
    *  \----------------/         \----------------/          |-------------.            *
    *    | Refers      L--------------------------------.     |             |            *
    *    v                     Refers                   v     v             |            *
    *  /----------------\                         /----------------\        |            *
    *  |  scud (basic)  |<------------------------|  stud (basic)  |        |            *
    *  \----------------/      Registers          | (Upgrades to   |        |            *
    *                                             |   Lifetime)    |        v            *
    *                                             \----------------/   /--------------\  *
    *                                                         L------->| pleb (Basic) |  *
    *                                                          Refers  \--------------/  *
    *                                                                                    *
    */
   ACTOR(life);
   ACTOR(reggie);
   PREP_ACTOR(ann);
   PREP_ACTOR(scud);
   PREP_ACTOR(dumy);
   PREP_ACTOR(stud);
   PREP_ACTOR(pleb);
   transfer(account_id_type(), life_id, asset(100000000));
   transfer(account_id_type(), reggie_id, asset(100000000));
   upgrade_to_lifetime_member(life_id);
   upgrade_to_lifetime_member(reggie_id);
   enable_fees(10000);
   const auto& fees = db.get_global_properties().parameters.current_fees;

#define CustomRegisterActor(actor_name, registrar_name, referrer_name, referrer_rate) \
   account_id_type actor_name ## _id; \
   { \
      account_create_operation op; \
      op.registrar = registrar_name ## _id; \
      op.referrer = referrer_name ## _id; \
      op.referrer_percent = referrer_rate*GRAPHENE_1_PERCENT; \
      op.name = BOOST_PP_STRINGIZE(actor_name); \
      op.options.memo_key = actor_name ## _private_key.get_public_key(); \
      op.active = authority(1, public_key_type(actor_name ## _private_key.get_public_key()), 1); \
      op.owner = op.active; \
      op.fee = op.calculate_fee(fees); \
      trx.operations = {op}; \
      trx.sign( registrar_name ## _private_key); \
      actor_name ## _id = PUSH_TX( db, trx ).operation_results.front().get<object_id_type>(); \
      trx.clear(); \
   }

   CustomRegisterActor(ann, life, life, 75);

   transfer(life_id, ann_id, asset(1000000));
   upgrade_to_annual_member(ann_id);

   CustomRegisterActor(dumy, reggie, life, 75);
   CustomRegisterActor(stud, reggie, ann, 80);

   transfer(life_id, stud_id, asset(1000000));
   upgrade_to_lifetime_member(stud_id);

   CustomRegisterActor(pleb, reggie, stud, 95);
   CustomRegisterActor(scud, stud, ann, 80);

   generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
   enable_fees(10000);

   BOOST_CHECK_EQUAL(life_id(db).cashback_balance(db).balance.amount.value, 78000);
   BOOST_CHECK_EQUAL(reggie_id(db).cashback_balance(db).balance.amount.value, 34000);
   BOOST_CHECK_EQUAL(ann_id(db).cashback_balance(db).balance.amount.value, 40000);
   BOOST_CHECK_EQUAL(stud_id(db).cashback_balance(db).balance.amount.value, 8000);

   transfer(stud_id, scud_id, asset(500000));
   transfer(scud_id, pleb_id, asset(400000));
   transfer(pleb_id, dumy_id, asset(300000));
   transfer(dumy_id, reggie_id, asset(200000));

   generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

   BOOST_CHECK_EQUAL(life_id(db).cashback_balance(db).balance.amount.value, 87750);
   BOOST_CHECK_EQUAL(reggie_id(db).cashback_balance(db).balance.amount.value, 35500);
   BOOST_CHECK_EQUAL(ann_id(db).cashback_balance(db).balance.amount.value, 44000);
   BOOST_CHECK_EQUAL(stud_id(db).cashback_balance(db).balance.amount.value, 24750);

   generate_blocks(ann_id(db).membership_expiration_date);
   generate_block();
   enable_fees(10000);

   //ann's membership has expired, so scud's fee should go up to life instead.
   transfer(scud_id, pleb_id, asset(10));

   generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

   BOOST_CHECK_EQUAL(life_id(db).cashback_balance(db).balance.amount.value, 94750);
   BOOST_CHECK_EQUAL(reggie_id(db).cashback_balance(db).balance.amount.value, 35500);
   BOOST_CHECK_EQUAL(ann_id(db).cashback_balance(db).balance.amount.value, 44000);
   BOOST_CHECK_EQUAL(stud_id(db).cashback_balance(db).balance.amount.value, 25750);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(bulk_discount)
{ try {
   ACTOR(nathan);
   // Give nathan ALLLLLL the money!
   transfer(GRAPHENE_COMMITTEE_ACCOUNT, nathan_id, db.get_balance(GRAPHENE_COMMITTEE_ACCOUNT, asset_id_type()));
   enable_fees(GRAPHENE_BLOCKCHAIN_PRECISION*10);
   upgrade_to_lifetime_member(nathan_id);
   share_type new_fees;
   while( nathan_id(db).statistics(db).lifetime_fees_paid + new_fees < GRAPHENE_DEFAULT_BULK_DISCOUNT_THRESHOLD_MIN )
   {
      transfer(nathan_id, GRAPHENE_COMMITTEE_ACCOUNT, asset(1));
      new_fees += transfer_operation().calculate_fee(db.current_fee_schedule());
   }
   generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
   enable_fees(GRAPHENE_BLOCKCHAIN_PRECISION*10);
   auto old_cashback = nathan_id(db).cashback_balance(db).balance;

   transfer(nathan_id, GRAPHENE_COMMITTEE_ACCOUNT, asset(1));
   generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
   enable_fees(GRAPHENE_BLOCKCHAIN_PRECISION*10);

   BOOST_CHECK_EQUAL(nathan_id(db).cashback_balance(db).balance.amount.value,
                     old_cashback.amount.value + GRAPHENE_BLOCKCHAIN_PRECISION * 8);

   new_fees = 0;
   while( nathan_id(db).statistics(db).lifetime_fees_paid + new_fees < GRAPHENE_DEFAULT_BULK_DISCOUNT_THRESHOLD_MAX )
   {
      transfer(nathan_id, GRAPHENE_COMMITTEE_ACCOUNT, asset(1));
      new_fees += transfer_operation().calculate_fee(db.current_fee_schedule());
   }
   generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
   enable_fees(GRAPHENE_BLOCKCHAIN_PRECISION*10);
   old_cashback = nathan_id(db).cashback_balance(db).balance;

   transfer(nathan_id, GRAPHENE_COMMITTEE_ACCOUNT, asset(1));
   generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

   BOOST_CHECK_EQUAL(nathan_id(db).cashback_balance(db).balance.amount.value,
                     old_cashback.amount.value + GRAPHENE_BLOCKCHAIN_PRECISION * 9);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
