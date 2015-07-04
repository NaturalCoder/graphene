#pragma once

#include <graphene/chain/database.hpp>
#include <graphene/chain/transaction.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>
#include <graphene/chain/balance_object.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/exceptions.hpp>

namespace graphene { namespace chain {

class balance_claim_evaluator : public evaluator<balance_claim_evaluator>
{
public:
   typedef balance_claim_operation operation_type;

   const balance_object* balance = nullptr;
   asset amount_withdrawn;

   asset do_evaluate(const balance_claim_operation& op)
   {
      database& d = db();
      balance = &op.balance_to_claim(d);

      FC_ASSERT(op.balance_owner_key == balance->owner ||
                pts_address(op.balance_owner_key, false, 56) == balance->owner ||
                pts_address(op.balance_owner_key, true, 56) == balance->owner ||
                pts_address(op.balance_owner_key, false, 0) == balance->owner ||
                pts_address(op.balance_owner_key, true, 0) == balance->owner,
                "balance_owner_key does not match balance's owner");
      if( !(d.get_node_properties().skip_flags & (database::skip_authority_check |
                                                  database::skip_transaction_signatures)) )
         FC_ASSERT(trx_state->signed_by(op.balance_owner_key));
      FC_ASSERT(op.total_claimed.asset_id == balance->asset_type());

      if( balance->vesting_policy.valid() ) {
         FC_ASSERT(op.total_claimed.amount == 0);
         if( d.head_block_time() - balance->last_claim_date < fc::days(1) )
            FC_THROW_EXCEPTION(balance_claimed_too_often,
                               "Genesis vesting balances may not be claimed more than once per day.");
         return amount_withdrawn = balance->vesting_policy->get_allowed_withdraw({balance->balance,
                                                                                  d.head_block_time(),
                                                                                  {}});
      }

      FC_ASSERT(op.total_claimed == balance->balance);
      return amount_withdrawn = op.total_claimed;
   }

   /**
    * @note the fee is always 0 for this particular operation because once the
    * balance is claimed it frees up memory and it cannot be used to spam the network
    */
   asset do_apply(const balance_claim_operation& op)
   {
      database& d = db();

      if( balance->vesting_policy.valid() && amount_withdrawn < balance->balance )
         d.modify(*balance, [&](balance_object& b) {
            b.vesting_policy->on_withdraw({b.balance, d.head_block_time(), amount_withdrawn});
            b.balance -= amount_withdrawn;
            b.last_claim_date = d.head_block_time();
         });
      else
         d.remove(*balance);

      d.adjust_balance(op.deposit_to_account, amount_withdrawn);
      return amount_withdrawn;
   }
};

} } // graphene::chain
