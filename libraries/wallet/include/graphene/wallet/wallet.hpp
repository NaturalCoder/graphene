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
#pragma once

#include <graphene/app/api.hpp>
#include <graphene/chain/address.hpp>
#include <graphene/utilities/key_conversion.hpp>

using namespace graphene::app;
using namespace graphene::chain;
using namespace graphene::utilities;
using namespace std;

namespace fc
{
   void to_variant(const account_multi_index_type& accts, variant& vo);
   void from_variant(const variant &var, account_multi_index_type &vo);
}

namespace graphene { namespace wallet {

typedef uint16_t transaction_handle_type;

/**
 * This class takes a variant and turns it into an object
 * of the given type, with the new operator.
 */

object* create_object( const variant& v );

struct plain_keys
{
   map<public_key_type, string>  keys;
   fc::sha512                    checksum;
};

struct wallet_data
{
   account_multi_index_type my_accounts;
   /// @return IDs of all accounts in @ref my_accounts
   vector<object_id_type> my_account_ids()const
   {
      vector<object_id_type> ids;
      ids.reserve(my_accounts.size());
      std::transform(my_accounts.begin(), my_accounts.end(), std::back_inserter(ids),
                     [](const account_object& ao) { return ao.id; });
      return ids;
   }
   /// Add acct to @ref my_accounts, or update it if it is already in @ref my_accounts
   /// @return true if the account was newly inserted; false if it was only updated
   bool update_account(const account_object& acct)
   {
      auto& idx = my_accounts.get<by_id>();
      auto itr = idx.find(acct.get_id());
      if( itr != idx.end() )
      {
         idx.replace(itr, acct);
         return false;
      } else {
         idx.insert(acct);
         return true;
      }
   }

   /** encrypted keys */
   vector<char>              cipher_keys;

   /** map an account to a set of extra keys that have been imported for that account */
   map<account_id_type, set<public_key_type> >  extra_keys;

   // map of account_name -> base58_private_key for
   //    incomplete account regs
   map<string, vector<string> > pending_account_registrations;
   map<string, string> pending_witness_registrations;

   string                    ws_server = "ws://localhost:8090";
   string                    ws_user;
   string                    ws_password;
};

namespace detail {
class wallet_api_impl;
}

/**
 * This wallet assumes it is connected to the database server with a high-bandwidth, low-latency connection and
 * performs minimal caching. This API could be provided locally to be used by a web interface.
 */
class wallet_api
{
   public:
      wallet_api(fc::api<login_api> rapi);
      virtual ~wallet_api();

      bool copy_wallet_file( string destination_filename );

      fc::ecc::private_key derive_private_key(const std::string& prefix_string, int sequence_number) const;

      variant                           info();
      optional<signed_block>            get_block( uint32_t num );
      /** Returns the number of accounts registered on the blockchain
       * @returns the number of registered accounts
       */
      uint64_t                          get_account_count()const;
      /** Lists all accounts controlled by this wallet.
       * This returns a list of the full account objects for all accounts whose private keys 
       * we possess.
       * @returns a list of account objects
       */
      vector<account_object>            list_my_accounts();
      /** Lists all accounts registered in the blockchain.
       * This returns a list of all account names and their account ids, sorted by account name.
       *
       * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all accounts,
       * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
       * the last account name returned as the \c lowerbound for the next \c list_accounts() call.
       *
       * @param lowerbound the name of the first account to return.  If the named account does not exist, 
       *                   the list will start at the account that comes after \c lowerbound
       * @param limit the maximum number of accounts to return (max: 1000)
       * @returns a list of accounts mapping account names to account ids
       */
      map<string,account_id_type>       list_accounts(const string& lowerbound, uint32_t limit);
      /** List the balances of an account.
       * Each account can have multiple balances, one for each type of asset owned by that 
       * account.  The returned list will only contain assets for which the account has a
       * nonzero balance
       * @param id the name or id of the account whose balances you want
       * @returns a list of the given account's balances
       */
      vector<asset>                     list_account_balances(const string& id);
      /** Lists all assets registered on the blockchain.
       * 
       * To list all assets, pass the empty string \c "" for the lowerbound to start
       * at the beginning of the list, and iterate as necessary.
       *
       * @param lowerbound  the symbol of the first asset to include in the list.
       * @param limit the maximum number of assets to return (max: 100)
       * @returns the list of asset objects, ordered by symbol
       */
      vector<asset_object>              list_assets(const string& lowerbound, uint32_t limit)const;
      
      /** Returns the most recent operations on the named account.
       *
       * This returns a list of operation history objects, which describe activity on the account.
       *
       * @note this API doesn't give a way to retrieve more than the most recent 100 transactions,
       *       you can interface directly with the blockchain to get more history
       * @param name the name or id of the account
       * @param limit the number of entries to return (starting from the most recent) (max 100)
       * @returns a list of \c operation_history_objects
       */
      vector<operation_history_object>  get_account_history(string name, int limit)const;


      vector<bucket_object>             get_market_history(string symbol, string symbol2, uint32_t bucket)const;
      vector<limit_order_object>        get_limit_orders(string a, string b, uint32_t limit)const;
      vector<call_order_object>         get_call_orders(string a, uint32_t limit)const;
      vector<force_settlement_object>   get_settle_orders(string a, uint32_t limit)const;
      
      /** Returns the block chain's slowly-changing settings.
       * This object contains all of the properties of the blockchain that are fixed
       * or that change only once per maintenance interval (daily) such as the
       * current list of witnesses, delegates, block interval, etc.
       * @see \c get_dynamic_global_properties() for frequently changing properties
       * @returns the global properties
       */
      global_property_object            get_global_properties() const;

      /** Returns the block chain's rapidly-changing properties.
       * The returned object contains information that changes every block interval
       * such as the head block number, the next witness, etc.
       * @see \c get_global_properties() for less-frequently changing properties
       * @returns the dynamic global properties
       */
      dynamic_global_property_object    get_dynamic_global_properties() const;

      /** Returns information about the given account.
       *
       * @param account_name_or_id the name or id of the account to provide information about
       * @returns the public account data stored in the blockchain
       */
      account_object                    get_account(string account_name_or_id) const;

      /** Returns information about the given asset.
       * @param asset_name_or_id the symbol or id of the asset in question
       * @returns the information about the asset stored in the block chain
       */
      asset_object                      get_asset(string asset_name_or_id) const;

      /** Returns the BitAsset-specific data for a given asset.
       * Market-issued assets's behavior are determined both by their "BitAsset Data" and
       * their basic asset data, as returned by \c get_asset().
       * @param asset_name_or_id the symbol or id of the BitAsset in question
       * @returns the BitAsset-specific data for this asset
       */
      asset_bitasset_data_object        get_bitasset_data(string asset_name_or_id)const;

      /** Lookup the id of a named account.
       * @param account_name_or_id the name of the account to look up
       * @returns the id of the named account
       */
      account_id_type                   get_account_id(string account_name_or_id) const;

      /**
       * Lookup the id of a named asset.
       * @param asset_name_or_id the symbol of an asset to look up
       * @returns the id of the given asset
       */
      asset_id_type                     get_asset_id(string asset_name_or_id) const;

      /**
       * Returns the blockchain object corresponding to the given id.
       *
       * This generic function can be used to retrieve any object from the blockchain
       * that is assigned an ID.  Certain types of objects have specialized convenience 
       * functions to return their objects -- e.g., assets have \c get_asset(), accounts 
       * have \c get_account(), but this function will work for any object.
       *
       * @param id the id of the object to return
       * @returns the requested object
       */
      variant                           get_object(object_id_type id) const;

      /** Returns the current wallet filename.  
       *
       * This is the filename that will be used when automatically saving the wallet.
       *
       * @see set_wallet_filename()
       * @return the wallet filename
       */
      string                            get_wallet_filename() const;

      /**
       * @ingroup Transaction Builder API
       */
      transaction_handle_type begin_builder_transaction();
      /**
       * @ingroup Transaction Builder API
       */
      void add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op);
      /**
       * @ingroup Transaction Builder API
       */
      void replace_operation_in_builder_transaction(transaction_handle_type handle,
                                                    unsigned operation_index,
                                                    const operation& new_op);
      /**
       * @ingroup Transaction Builder API
       */
      asset set_fees_on_builder_transaction(transaction_handle_type handle, string fee_asset = GRAPHENE_SYMBOL);
      /**
       * @ingroup Transaction Builder API
       */
      transaction preview_builder_transaction(transaction_handle_type handle);
      /**
       * @ingroup Transaction Builder API
       */
      signed_transaction sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast = true);
      /**
       * @ingroup Transaction Builder API
       */
      signed_transaction propose_builder_transaction(transaction_handle_type handle,
                                                     time_point_sec expiration = time_point::now() + fc::minutes(1),
                                                     uint32_t review_period_seconds = 0, bool broadcast = true);
      /**
       * @ingroup Transaction Builder API
       */
      void remove_builder_transaction(transaction_handle_type handle);

      /** Checks whether the wallet has just been created and has not yet had a password set.
       *
       * Calling \c set_password will transition the wallet to the locked state.
       * @return true if the wallet is new
       * @ingroup Wallet Management
       */
      bool    is_new()const;
      
      /** Checks whether the wallet is locked (is unable to use its private keys).  
       *
       * This state can be changed by calling \c lock() or \c unlock().
       * @return true if the wallet is locked
       * @ingroup Wallet Management
       */
      bool    is_locked()const;
      
      /** Locks the wallet immediately.
       * @ingroup Wallet Management
       */
      void    lock();
      
      /** Unlocks the wallet.  
       *
       * The wallet remain unlocked until the \c lock is called
       * or the program exits.
       * @param password the password previously set with \c set_password()
       * @ingroup Wallet Management
       */
      void    unlock(string password);
      
      /** Sets a new password on the wallet.
       *
       * The wallet must be either 'new' or 'unlocked' to
       * execute this command.
       * @ingroup Wallet Management
       */
      void    set_password(string password);

      /** Dumps all private keys owned by the wallet.
       *
       * The keys are printed in WIF format.  You can import these keys into another wallet
       * using \c import_key()
       * @returns a map containing the private keys, indexed by their public key 
       */
      map<public_key_type, string> dump_private_keys();

      /** Returns a list of all commands supported by the wallet API.
       *
       * This lists each command, along with its arguments and return types.
       * For more detailed help on a single command, use \c get_help()
       *
       * @returns a multi-line string suitable for displaying on a terminal
       */
      string  help()const;

      /** Returns detailed help on a single API command.
       * @param method the name of the API command you want help with
       * @returns a multi-line string suitable for displaying on a terminal
       */
      string  gethelp(const string& method)const;

      /** Loads a specified Graphene wallet.
       *
       * The current wallet is closed before the new wallet is loaded.
       *
       * @warning This does not change the filename that will be used for future
       * wallet writes, so this may cause you to overwrite your original
       * wallet unless you also call \c set_wallet_filename()
       *
       * @param wallet_filename the filename of the wallet JSON file to load.
       *                        If \c wallet_filename is empty, it reloads the
       *                        existing wallet file
       * @returns true if the specified wallet is loaded
       */
      bool    load_wallet_file(string wallet_filename = "");

      /** Saves the current wallet to the given filename.
       * 
       * @warning This does not change the wallet filename that will be used for future
       * writes, so think of this function as 'Save a Copy As...' instead of
       * 'Save As...'.  Use \c set_wallet_filename() to make the filename
       * persist.
       * @param wallet_filename the filename of the new wallet JSON file to create
       *                        or overwrite.  If \c wallet_filename is empty,
       *                        save to the current filename.
       */
      void    save_wallet_file(string wallet_filename = "");

      /** Sets the wallet filename used for future writes.
       *
       * This does not trigger a save, it only changes the default filename
       * that will be used the next time a save is triggered.
       *
       * @param wallet_filename the new filename to use for future saves
       */
      void    set_wallet_filename(string wallet_filename);

      /** Suggests a safe brain key to use for creating your account.
       * \c create_account_with_brain_key() requires you to specify a 'brain key',
       * a long passphrase that provides enough entropy to generate cyrptographic
       * keys.  This function will suggest a suitably random string that should
       * be easy to write down (and, with effort, memorize).
       * @returns a suggested brain_key
       */
      string  suggest_brain_key()const;

      /** Converts a signed_transaction in JSON form to its binary representation.
       *
       * TODO: I don't see a broadcast_transaction() function, do we need one?
       *
       * @param tx the transaction to serialize
       * @returns the binary form of the transaction.  It will not be hex encoded, 
       *          this returns a raw string that may have null characters embedded 
       *          in it
       */
      string serialize_transaction(signed_transaction tx) const;

      /** Imports the private key for an existing account.
       *
       * The private key must match either an owner key or an active key for the
       * named account.  
       *
       * @see dump_private_keys()
       *
       * @param account_name_or_id the account owning the key
       * @param wif_key the private key in WIF format
       * @returns true if the key was imported
       */
      bool import_key(string account_name_or_id, string wif_key);

      /**
       * This call will construct a transaction that will claim all balances controled
       * by wif_keys and deposit them into the given account.
       */
      signed_transaction import_balance( string account_name_or_id, const vector<string>& wif_keys, bool broadcast );

      /** Transforms a brain key to reduce the chance of errors when re-entering the key from memory.
       *
       * This takes a user-supplied brain key and normalizes it into the form used
       * for generating private keys.  In particular, this upper-cases all ASCII characters
       * and collapses multiple spaces into one.
       * @param s the brain key as supplied by the user
       * @returns the brain key in its normalized form
       */
      string normalize_brain_key(string s) const;

      /** Registers a third party's account on the blockckain.
       *
       * This function is used to register an account for which you do not own the private keys.
       * When acting as a registrar, an end user will generate their own private keys and send
       * you the public keys.  The registrar will use this function to register the account
       * on behalf of the end user.
       *
       * @see create_account_with_brain_key()
       *
       * @param name the name of the account, must be unique on the blockchain.  Shorter names
       *             are more expensive to register; the rules are still in flux, but in general
       *             names of more than 8 characters with at least one digit will be cheap.
       * @param owner the owner key for the new account
       * @param active the active key for the new account
       * @param registrar_account the account which will pay the fee to register the user
       * @param referrer_account the account who is acting as a referrer, and may receive a
       *                         portion of the user's transaction fees.  This can be the
       *                         same as the registrar_account if there is no referrer.
       * @param referrer_percent the percentage (0 - 100) of the new user's transaction fees
       *                         not claimed by the blockchain that will be distributed to the 
       *                         referrer; the rest will be sent to the registrar
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction registering the account
       */
      signed_transaction register_account(string name,
                                          public_key_type owner,
                                          public_key_type active,
                                          string  registrar_account,
                                          string  referrer_account,
                                          uint8_t referrer_percent,
                                          bool broadcast = false);

      /**
       *  Upgrades an account to prime status.
       *  This makes the account holder a 'lifetime member'.
       *
       *  @todo there is no option for annual membership
       *  @param name the name or id of the account to upgrade
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction upgrading the account
       */
      signed_transaction upgrade_account(string name, bool broadcast);

      /** Creates a new account and registers it on the blockchain.
       *
       * @todo why no referrer_percent here?
       *
       * @see suggest_brain_key()
       * @see register_account()
       *
       * @param brain_key the brain key used for generating the account's private keys
       * @param account_name the name of the account, must be unique on the blockchain.  Shorter names
       *                     are more expensive to register; the rules are still in flux, but in general
       *                     names of more than 8 characters with at least one digit will be cheap.
       * @param registrar_account the account which will pay the fee to register the user
       * @param referrer_account the account who is acting as a referrer, and may receive a
       *                         portion of the user's transaction fees.  This can be the
       *                         same as the registrar_account if there is no referrer.
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction registering the account
       */
      signed_transaction create_account_with_brain_key(string brain_key,
                                                       string account_name,
                                                       string registrar_account,
                                                       string referrer_account,
                                                       bool broadcast = false);

      /** Transfer an amount from one account to another.
       * @param from the name or id of the account sending the funds
       * @param to the name or id of the account receiving the funds
       * @param amount the amount to send (in nominal units -- to send half of a BTS, specify 0.5)
       * @param asset_symbol the symbol or id of the asset to send
       * @param memo a memo to attach to the transaction.  The memo will be encrypted in the 
       *             transaction and readable for the receiver.  There is no length limit
       *             other than the limit imposed by maximum transaction size, but transaction
       *             increase with transaction size
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction transferring funds
       */
      signed_transaction transfer(string from,
                                  string to,
                                  string amount,
                                  string asset_symbol,
                                  string memo,
                                  bool broadcast = false);

      /** Place a limit order attempting to sell one asset for another.
       *
       * Buying and selling are the same operation on Graphene; if you want to buy BTS 
       * with USD, you should sell USD for BTS.
       *
       * The blockchain will attempt to sell the \c symbol_to_sell for as
       * much \c symbol_to_receive as possible, as long as the price is at 
       * least \c min_to_receive / \c amount_to_sell.   
       *
       * In addition to the transaction fees, market fees will apply as specified 
       * by the issuer of both the selling asset and the receiving asset as
       * a percentage of the amount exchanged.
       *
       * If either the selling asset or the receiving asset is whitelist
       * restricted, the order will only be created if the seller is on
       * the whitelist of the restricted asset type.
       *
       * Market orders are matched in the order they are included
       * in the block chain.
       *
       * @todo Allow order expiration to be set here.  Document default/max expiration time
       *
       * @param seller_account the account providing the asset being sold, and which will 
       *                       receive the proceeds of the sale.
       * @param amount_to_sell the amount of the asset being sold to sell (in nominal units)
       * @param symbol_to_sell the name or id of the asset to sell
       * @param min_to_receive the minimum amount you are willing to receive in return for
       *                       selling the entire amount_to_sell
       * @param symbol_to_receive the name or id of the asset you wish to receive
       * @param timeout_sec if the order does not fill immediately, this is the length of 
       *                    time the order will remain on the order books before it is 
       *                    cancelled and the un-spent funds are returned to the seller's 
       *                    account
       * @param fill_or_kill if true, the order will only be included in the blockchain
       *                     if it is filled immediately; if false, an open order will be
       *                     left on the books to fill any amount that cannot be filled
       *                     immediately.
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction selling the funds
       */
      signed_transaction sell_asset(string seller_account,
                                    string amount_to_sell,
                                    string   symbol_to_sell,
                                    string min_to_receive,
                                    string   symbol_to_receive,
                                    uint32_t timeout_sec = 0,
                                    bool     fill_or_kill = false,
                                    bool     broadcast = false);

      /** Borrow an asset or update the debt/collateral ratio for the loan.
       *
       * This is the first step in shorting an asset.  Call \c sell_asset() to complete the short.
       *
       * @param borrower_name the name or id of the account associated with the transaction.
       * @param amount_to_borrow the amount of the asset being borrowed.  Make this value
       *                         negative to pay back debt.
       * @param asset_symbol the symbol or id of the asset being borrowed.
       * @param amount_of_collateral the amount of the backing asset to add to your collateral
       *        position.  Make this negative to claim back some of your collateral.
       *        The backing asset is defined in the \c bitasset_options for the asset being borrowed.
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction borrowing the asset
       */
      signed_transaction borrow_asset(string borrower_name, string amount_to_borrow, string asset_symbol,
                                      string amount_of_collateral, bool broadcast = false);

      /** Creates a new user-issued or market-issued asset.
       *
       * Many options can be changed later using \c update_asset()
       *
       * Right now this function is difficult to use because you must provide raw JSON data
       * structures for the options objects, and those include prices and asset ids.
       *
       * @param issuer the name or id of the account who will pay the fee and become the 
       *               issuer of the new asset.  This can be updated later
       * @param symbol the ticker symbol of the new asset
       * @param precision the number of digits of precision to the right of the decimal point,
       *                  must be less than or equal to 12
       * @param common asset options required for all new assets.
       *               Note that core_exchange_rate technically needs to store the asset ID of 
       *               this new asset. Since this ID is not known at the time this operation is 
       *               created, create this price as though the new asset has instance ID 1, and
       *               the chain will overwrite it with the new asset's ID.
       * @param bitasset_opts options specific to BitAssets.  This may be null unless the
       *               \c market_issued flag is set in common.flags
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction creating a new asset
       */
      signed_transaction create_asset(string issuer,
                                      string symbol,
                                      uint8_t precision,
                                      asset_object::asset_options common,
                                      fc::optional<asset_object::bitasset_options> bitasset_opts,
                                      bool broadcast = false);

      /** Issue new shares of an asset.
       *
       * @param to_account the name or id of the account to receive the new shares
       * @param amount the amount to issue, in nominal units
       * @param symbol the ticker symbol of the asset to issue
       * @param memo a memo to include in the transaction, readable by the recipient
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction issuing the new shares
       */
      signed_transaction issue_asset(string to_account, string amount,
                                     string symbol,
                                     string memo,
                                     bool broadcast = false);

      /** Update the core options on an asset.
       * There are a number of options which all assets in the network use. These options are 
       * enumerated in the asset_object::asset_options struct. This command is used to update 
       * these options for an existing asset.
       *
       * @note This operation cannot be used to update BitAsset-specific options. For these options,
       * \c update_bitasset() instead.
       *
       * @param symbol the name or id of the asset to update
       * @param new_issuer if changing the asset's issuer, the name or id of the new issuer.
       *                   null if you wish to remain the issuer of the asset
       * @param new_options the new asset_options object, which will entirely replace the existing
       *                    options.
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction updating the asset
       */
      signed_transaction update_asset(string symbol,
                                      optional<string> new_issuer,
                                      asset_object::asset_options new_options,
                                      bool broadcast = false);

      /** Update the options specific to a BitAsset.
       *
       * BitAssets have some options which are not relevant to other asset types. This operation is used to update those
       * options an an existing BitAsset.
       *
       * @see update_asset()
       *
       * @param symbol the name or id of the asset to update, which must be a market-issued asset
       * @param new_options the new bitasset_options object, which will entirely replace the existing
       *                    options.
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction updating the bitasset
       */
      signed_transaction update_bitasset(string symbol,
                                         asset_object::bitasset_options new_options,
                                         bool broadcast = false);

      /** Update the set of feed-producing accounts for a BitAsset.
       *
       * BitAssets have price feeds selected by taking the median values of recommendations from a set of feed producers.
       * This command is used to specify which accounts may produce feeds for a given BitAsset.
       * @param symbol the name or id of the asset to update
       * @param new_feed_producers a list of account names or ids which are authorized to produce feeds for the asset.
       *                           this list will completely replace the existing list
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction updating the bitasset's feed producers
       */
      signed_transaction update_asset_feed_producers(string symbol,
                                                     flat_set<string> new_feed_producers,
                                                     bool broadcast = false);
      
      /** Publishes a price feed for the named asset.
       *
       * Price feed providers use this command to publish their price feeds for market-issued assets. A price feed is
       * used to tune the market for a particular market-issued asset. For each value in the feed, the median across all
       * delegate feeds for that asset is calculated and the market for the asset is configured with the median of that
       * value.
       *
       * The feed object in this command contains three prices: a call price limit, a short price limit, and a settlement price.
       * The call limit price is structured as (collateral asset) / (debt asset) and the short limit price is structured
       * as (asset for sale) / (collateral asset). Note that the asset IDs are opposite to eachother, so if we're
       * publishing a feed for USD, the call limit price will be CORE/USD and the short limit price will be USD/CORE. The
       * settlement price may be flipped either direction, as long as it is a ratio between the market-issued asset and
       * its collateral.
       *
       * @param publishing_account the account publishing the price feed
       * @param symbol the name or id of the asset whose feed we're publishing
       * @param feed the price_feed object containing the three prices making up the feed
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction updating the price feed for the given asset
       */
      signed_transaction publish_asset_feed(string publishing_account,
                                            string symbol,
                                            price_feed feed,
                                            bool broadcast = false);

      /** Pay into the fee pool for the given asset.
       *
       * User-issued assets can optionally have a pool of the core asset which is 
       * automatically used to pay transaction fees for any transaction using that
       * asset (using the asset's core exchange rate).
       *
       * This command allows anyone to deposit the core asset into this fee pool.
       *
       * @param from the name or id of the account sending the core asset
       * @param symbol the name or id of the asset whose fee pool you wish to fund
       * @param amount the amount of the core asset to deposit
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction funding the fee pool
       */
      signed_transaction fund_asset_fee_pool(string from,
                                             string symbol,
                                             string amount,
                                             bool broadcast = false);

      /** Burns the given user-issued asset.
       *
       * This command burns the user-issued asset to reduce the amount in circulation.
       * @note you cannot burn market-issued assets.
       * @param from the account containing the asset you wish to burn
       * @param amount the amount to burn, in nominal units
       * @param symbol the name or id of the asset to burn
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction burning the asset
       */
      signed_transaction reserve_asset(string from,
                                    string amount,
                                    string symbol,
                                    bool broadcast = false);

      /** Forces a global settling of the given asset (black swan or prediction markets).
       *
       * In order to use this operation, asset_to_settle must have the global_settle flag set
       *
       * When this operation is executed all balances are converted into the backing asset at the
       * settle_price and all open margin positions are called at the settle price.  If this asset is
       * used as backing for other bitassets, those bitassets will be force settled at their current
       * feed price.
       *
       * @note this operation is used only by the asset issuer, \c settle_asset() may be used by 
       *       any user owning the asset
       *
       * @param symbol the name or id of the asset to force settlement on
       * @param settle_price the price at which to settle
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction settling the named asset
       */
      signed_transaction global_settle_asset(string symbol,
                                             price settle_price,
                                             bool broadcast = false);

      /** Schedules a market-issued asset for automatic settlement.
       *
       * Holders of market-issued assests may request a forced settlement for some amount of their asset. This means that
       * the specified sum will be locked by the chain and held for the settlement period, after which time the chain will
       * choose a margin posision holder and buy the settled asset using the margin's collateral. The price of this sale
       * will be based on the feed price for the market-issued asset being settled. The exact settlement price will be the
       * feed price at the time of settlement with an offset in favor of the margin position, where the offset is a
       * blockchain parameter set in the global_property_object.
       *
       * @param account_to_settle the name or id of the account owning the asset
       * @param amount_to_settle the amount of the named asset to schedule for settlement
       * @param symbol the name or id of the asset to settlement on
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction settling the named asset
       */
      signed_transaction settle_asset(string account_to_settle,
                                      string amount_to_settle,
                                      string symbol,
                                      bool broadcast = false);

      /** Whitelist and blacklist accounts, primarily for transacting in whitelisted assets.
       *
       * Accounts can freely specify opinions about other accounts, in the form of either whitelisting or blacklisting
       * them. This information is used in chain validation only to determine whether an account is authorized to transact
       * in an asset type which enforces a whitelist, but third parties can use this information for other uses as well,
       * as long as it does not conflict with the use of whitelisted assets.
       *
       * An asset which enforces a whitelist specifies a list of accounts to maintain its whitelist, and a list of
       * accounts to maintain its blacklist. In order for a given account A to hold and transact in a whitelisted asset S,
       * A must be whitelisted by at least one of S's whitelist_authorities and blacklisted by none of S's
       * blacklist_authorities. If A receives a balance of S, and is later removed from the whitelist(s) which allowed it
       * to hold S, or added to any blacklist S specifies as authoritative, A's balance of S will be frozen until A's
       * authorization is reinstated.
       *
       * @param authorizing_account the account who is doing the whitelisting
       * @param account_to_list the account being whitelisted
       * @param new_listing_status the new whitelisting status
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction changing the whitelisting status
       */
      signed_transaction whitelist_account(string authorizing_account,
                                           string account_to_list,
                                           account_whitelist_operation::account_listing new_listing_status,
                                           bool broadcast = false);

      /** Creates a delegate object owned by the given account.
       *
       * An account can have at most one delegate object.
       *
       * @param owner_account the name or id of the account which is creating the delegate
       * @param url a URL to include in the delegate record in the blockchain.  Clients may
       *            display this when showing a list of delegates.  May be blank.
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction registering a delegate
       */
      signed_transaction create_delegate(string owner_account,
                                         string url, 
                                         bool broadcast = false);

      /** Lists all witnesses registered in the blockchain.
       * This returns a list of all account names that own witnesses, and the associated witness id,
       * sorted by name.  This lists witnesses whether they are currently voted in or not.
       *
       * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all witnesss,
       * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
       * the last witness name returned as the \c lowerbound for the next \c list_witnesss() call.
       *
       * @param lowerbound the name of the first witness to return.  If the named witness does not exist, 
       *                   the list will start at the witness that comes after \c lowerbound
       * @param limit the maximum number of witnesss to return (max: 1000)
       * @returns a list of witnesss mapping witness names to witness ids
       */
      map<string,witness_id_type>       list_witnesses(const string& lowerbound, uint32_t limit);

      /** Lists all delegates registered in the blockchain.
       * This returns a list of all account names that own delegates, and the associated delegate id,
       * sorted by name.  This lists delegates whether they are currently voted in or not.
       *
       * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all delegates,
       * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
       * the last delegate name returned as the \c lowerbound for the next \c list_delegates() call.
       *
       * @param lowerbound the name of the first delegate to return.  If the named delegate does not exist, 
       *                   the list will start at the delegate that comes after \c lowerbound
       * @param limit the maximum number of delegates to return (max: 1000)
       * @returns a list of delegates mapping delegate names to delegate ids
       */
      map<string, delegate_id_type>       list_delegates(const string& lowerbound, uint32_t limit);

      /** Returns information about the given witness.
       * @param owner_account the name or id of the witness account owner, or the id of the witness
       * @returns the information about the witness stored in the block chain
       */
      witness_object get_witness(string owner_account);

      /** Returns information about the given delegate.
       * @param owner_account the name or id of the delegate account owner, or the id of the delegate
       * @returns the information about the delegate stored in the block chain
       */
      delegate_object get_delegate(string owner_account);

      /** Creates a witness object owned by the given account.
       *
       * An account can have at most one witness object.
       *
       * @param owner_account the name or id of the account which is creating the witness
       * @param url a URL to include in the witness record in the blockchain.  Clients may
       *            display this when showing a list of witnesses.  May be blank.
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction registering a witness
       */
      signed_transaction create_witness(string owner_account,
                                        string url,
                                        bool broadcast = false);

      /** Vote for a given delegate.
       *
       * An account can publish a list of all delegatees they approve of.  This 
       * command allows you to add or remove delegatees from this list.
       * Each account's vote is weighted according to the number of shares of the
       * core asset owned by that account at the time the votes are tallied.
       *
       * @note you cannot vote against a delegate, you can only vote for the delegate
       *       or not vote for the delegate.
       *
       * @param voting_account the name or id of the account who is voting with their shares
       * @param delegate the name or id of the delegate' owner account
       * @param approve true if you wish to vote in favor of that delegate, false to 
       *                remove your vote in favor of that delegate
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed transaction changing your vote for the given delegate
       */
      signed_transaction vote_for_delegate(string voting_account,
                                           string delegate,
                                           bool approve,
                                           bool broadcast = false);

      /** Vote for a given witness.
       *
       * An account can publish a list of all witnesses they approve of.  This 
       * command allows you to add or remove witnesses from this list.
       * Each account's vote is weighted according to the number of shares of the
       * core asset owned by that account at the time the votes are tallied.
       *
       * @note you cannot vote against a witness, you can only vote for the witness
       *       or not vote for the witness.
       *
       * @param voting_account the name or id of the account who is voting with their shares
       * @param witness the name or id of the witness' owner account
       * @param approve true if you wish to vote in favor of that witness, false to 
       *                remove your vote in favor of that witness
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed transaction changing your vote for the given witness
       */
      signed_transaction vote_for_witness(string voting_account,
                                          string witness,
                                          bool approve,
                                          bool broadcast = false);

      /** Set the voting proxy for an account.
       *
       * If a user does not wish to take an active part in voting, they can choose
       * to allow another account to vote their stake.
       *
       * Setting a vote proxy does not remove your previous votes from the blockchain,
       * they remain there but are ignored.  If you later null out your vote proxy,
       * your previous votes will take effect again.
       *
       * This setting can be changed at any time.
       *
       * @param account_to_modify the name or id of the account to update
       * @param voting_account the name or id of an account authorized to vote account_to_modify's shares,
       *                       or null to vote your own shares
       *
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed transaction changing your vote proxy settings
       */
      signed_transaction set_voting_proxy(string account_to_modify,
                                          optional<string> voting_account,
                                          bool broadcast = false);

      /** Signs a transaction.
       *
       * Given a fully-formed transaction that is only lacking signatures, this signs
       * the transaction with the necessary keys and optionally broadcasts the transaction
       * @param tx the unsigned transaction
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed version of the transaction
       */
      signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false);

      void dbg_make_uia(string creator, string symbol);
      void dbg_make_mia(string creator, string symbol);
      void flood_network(string prefix, uint32_t number_of_transactions);

      std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const;


      fc::signal<void(bool)> lock_changed;
      std::shared_ptr<detail::wallet_api_impl> my;
      void encrypt_keys();
};

} }

FC_REFLECT( graphene::wallet::plain_keys, (keys)(checksum) )

FC_REFLECT( graphene::wallet::wallet_data,
            (my_accounts)
            (cipher_keys)
            (extra_keys)
            (pending_account_registrations)(pending_witness_registrations)
            (ws_server)
            (ws_user)
            (ws_password)
          )

FC_API( graphene::wallet::wallet_api,
        (help)
        (gethelp)
        (info)
        (begin_builder_transaction)
        (add_operation_to_builder_transaction)
        (replace_operation_in_builder_transaction)
        (set_fees_on_builder_transaction)
        (preview_builder_transaction)
        (sign_builder_transaction)
        (propose_builder_transaction)
        (remove_builder_transaction)
        (is_new)
        (is_locked)
        (lock)(unlock)(set_password)
        (dump_private_keys)
        (list_my_accounts)
        (list_accounts)
        (list_account_balances)
        (list_assets)
        (import_key)
        (import_balance)
        (suggest_brain_key)
        (register_account)
        (upgrade_account)
        (create_account_with_brain_key)
        (sell_asset)
        (borrow_asset)
        (transfer)
        (create_asset)
        (update_asset)
        (update_bitasset)
        (update_asset_feed_producers)
        (publish_asset_feed)
        (issue_asset)
        (get_asset)
        (get_bitasset_data)
        (fund_asset_fee_pool)
        (reserve_asset)
        (global_settle_asset)
        (settle_asset)
        (whitelist_account)
        (create_delegate)
        (get_witness)
        (get_delegate)
        (list_witnesses)
        (list_delegates)
        (create_witness)
        (vote_for_delegate)
        (vote_for_witness)
        (set_voting_proxy)
        (get_account)
        (get_account_id)
        (get_block)
        (get_account_count)
        (get_account_history)
        (get_market_history)
        (get_global_properties)
        (get_dynamic_global_properties)
        (get_object)
        (load_wallet_file)
        (normalize_brain_key)
        (get_limit_orders)
        (get_call_orders)
        (get_settle_orders)
        (save_wallet_file)
        (serialize_transaction)
        (sign_transaction)
        (dbg_make_uia)
        (dbg_make_mia)
        (flood_network)
      )
