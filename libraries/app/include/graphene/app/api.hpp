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
#include <graphene/chain/types.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/limit_order_object.hpp>
#include <graphene/chain/call_order_object.hpp>
#include <graphene/chain/delegate_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/balance_object.hpp>
#include <graphene/net/node.hpp>


#include <graphene/market_history/market_history_plugin.hpp>

#include <fc/api.hpp>

namespace graphene { namespace app {
   using namespace graphene::chain;
   using namespace graphene::market_history;

   class application;

   /**
    * @brief The database_api class implements the RPC API for the chain database.
    *
    * This API exposes accessors on the database which query state tracked by a blockchain validating node. This API is
    * read-only; all modifications to the database must be performed via transactions. Transactions are broadcast via
    * the @ref network_api.
    */
   class database_api
   {
      public:
         database_api(graphene::chain::database& db);
         ~database_api();
         /**
          * @brief Get the objects corresponding to the provided IDs
          * @param ids IDs of the objects to retrieve
          * @return The objects retrieved, in the order they are mentioned in ids
          *
          * If any of the provided IDs does not map to an object, a null variant is returned in its position.
          */
         fc::variants                      get_objects(const vector<object_id_type>& ids)const;
         /**
          * @brief Retrieve a block header
          * @param block_num Height of the block whose header should be returned
          * @return header of the referenced block, or null if no matching block was found
          */
         optional<block_header>            get_block_header(uint32_t block_num)const;
         /**
          * @brief Retrieve a full, signed block
          * @param block_num Height of the block to be returned
          * @return the referenced block, or null if no matching block was found
          */
         optional<signed_block>            get_block(uint32_t block_num)const;

         /** 
          * @brief used to fetch an individual transaction.
          */
         processed_transaction   get_transaction( uint32_t block_num, uint32_t trx_in_block )const;

         /**
          * @brief Retrieve the current @ref global_property_object
          */
         global_property_object            get_global_properties()const;
         /**
          * @brief Retrieve the current @ref dynamic_global_property_object
          */
         dynamic_global_property_object get_dynamic_global_properties()const;
         /**
          * @brief Get a list of accounts by ID
          * @param account_ids IDs of the accounts to retrieve
          * @return The accounts corresponding to the provided IDs
          *
          * This function has semantics identical to @ref get_objects
          */
         vector<optional<account_object>> get_accounts(const vector<account_id_type>& account_ids)const;
         /**
          * @brief Get a list of assets by ID
          * @param asset_ids IDs of the assets to retrieve
          * @return The assets corresponding to the provided IDs
          *
          * This function has semantics identical to @ref get_objects
          */
         vector<optional<asset_object>> get_assets(const vector<asset_id_type>& asset_ids)const;
         /**
          * @brief Get a list of accounts by name
          * @param account_names Names of the accounts to retrieve
          * @return The accounts holding the provided names
          *
          * This function has semantics identical to @ref get_objects
          */
         vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;
         /**
          * @brief Get a list of assets by symbol
          * @param asset_symbols Symbols of the assets to retrieve
          * @return The assets corresponding to the provided symbols
          *
          * This function has semantics identical to @ref get_objects
          */
         vector<optional<asset_object>> lookup_asset_symbols(const vector<string>& asset_symbols)const;

         /**
          * @brief Get an account's balances in various assets
          * @param id ID of the account to get balances for
          * @param assets IDs of the assets to get balances of
          * @return Balances of the account
          */
         vector<asset> get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const;
         /// Semantically equivalent to @ref get_account_balances, but takes a name instead of an ID.
         vector<asset> get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const;
         /**
          * @brief Get the total number of accounts registered with the blockchain
          */
         uint64_t get_account_count()const;
         /**
          * @brief Get names and IDs for registered accounts
          * @param lower_bound_name Lower bound of the first name to return
          * @param limit Maximum number of results to return -- must not exceed 1000
          * @return Map of account names to corresponding IDs
          */
         map<string,account_id_type> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;

         /**
          * @brief Get limit orders in a given market
          * @param a ID of asset being sold
          * @param b ID of asset being purchased
          * @param limit Maximum number of orders to retrieve
          * @return The limit orders, ordered from least price to greatest
          */
         vector<limit_order_object> get_limit_orders(asset_id_type a, asset_id_type b, uint32_t limit)const;
         /**
          * @brief Get call orders in a given asset
          * @param a ID of asset being called
          * @param limit Maximum number of orders to retrieve
          * @return The call orders, ordered from earliest to be called to latest
          */
         vector<call_order_object> get_call_orders(asset_id_type a, uint32_t limit)const;
         /**
          * @brief Get forced settlement orders in a given asset
          * @param a ID of asset being settled
          * @param limit Maximum number of orders to retrieve
          * @return The settle orders, ordered from earliest settlement date to latest
          */
         vector<force_settlement_object> get_settle_orders(asset_id_type a, uint32_t limit)const;

         /**
          * @brief Get assets alphabetically by symbol name
          * @param lower_bound_symbol Lower bound of symbol names to retrieve
          * @param limit Maximum number of assets to fetch (must not exceed 100)
          * @return The assets found
          */
         vector<asset_object> list_assets(const string& lower_bound_symbol, uint32_t limit)const;

         /**
          * @brief Get the delegate owned by a given account
          * @param account The ID of the account whose delegate should be retrieved
          * @return The delegate object, or null if the account does not have a delegate
          */
         fc::optional<delegate_object> get_delegate_by_account(account_id_type account)const;
         /**
          * @brief Get the witness owned by a given account
          * @param account The ID of the account whose witness should be retrieved
          * @return The witness object, or null if the account does not have a witness
          */
         fc::optional<witness_object> get_witness_by_account(account_id_type account)const;

         /**
          * @brief Get the total number of witnesses registered with the blockchain
          */
         uint64_t get_witness_count()const;

         /**
          * @brief Get names and IDs for registered witnesses
          * @param lower_bound_name Lower bound of the first name to return
          * @param limit Maximum number of results to return -- must not exceed 1000
          * @return Map of witness names to corresponding IDs
          */
         map<string, witness_id_type> lookup_witness_accounts(const string& lower_bound_name, uint32_t limit)const;
         
         /**
          * @brief Get names and IDs for registered delegates
          * @param lower_bound_name Lower bound of the first name to return
          * @param limit Maximum number of results to return -- must not exceed 1000
          * @return Map of delegate names to corresponding IDs
          */
         map<string, delegate_id_type> lookup_delegate_accounts(const string& lower_bound_name, uint32_t limit)const;
         
         /**
          * @brief Get a list of witnesses by ID
          * @param witness_ids IDs of the witnesses to retrieve
          * @return The witnesses corresponding to the provided IDs
          *
          * This function has semantics identical to @ref get_objects
          */
         vector<optional<witness_object>> get_witnesses(const vector<witness_id_type>& witness_ids)const;

         /**
          * @brief Get a list of delegates by ID
          * @param delegate_ids IDs of the delegates to retrieve
          * @return The delegates corresponding to the provided IDs
          *
          * This function has semantics identical to @ref get_objects
          */
         vector<optional<delegate_object>> get_delegates(const vector<delegate_id_type>& delegate_ids)const;

         /**
          * @group Push Notification Methods
          * These methods may be used to get push notifications whenever an object or market is changed
          * @{
          */
         /**
          * @brief Request notifications when some object(s) change
          * @param callback Callback method which is called with the new version of a changed object
          * @param ids The set of object IDs to watch
          */
         bool subscribe_to_objects(const std::function<void(const fc::variant&)>& callback,
                                   const vector<object_id_type>& ids);
         /**
          * @brief Stop receiving notifications for some object(s)
          * @param ids The set of object IDs to stop watching
          */
         bool unsubscribe_from_objects(const vector<object_id_type>& ids);
         /**
          * @brief Request notification when the active orders in the market between two assets changes
          * @param callback Callback method which is called when the market changes
          * @param a First asset ID
          * @param b Second asset ID
          *
          * Callback will be passed a variant containing a vector<pair<operation, operation_result>>. The vector will
          * contain, in order, the operations which changed the market, and their results.
          */
         void subscribe_to_market(std::function<void(const variant&)> callback,
                                  asset_id_type a, asset_id_type b);
         /**
          * @brief Unsubscribe from updates to a given market
          * @param a First asset ID
          * @param b Second asset ID
          */
         void unsubscribe_from_market(asset_id_type a, asset_id_type b);
         /**
          * @brief Stop receiving any notifications
          *
          * This unsubscribes from all subscribed markets and objects.
          */
         void cancel_all_subscriptions()
         { _subscriptions.clear(); _market_subscriptions.clear(); }
         ///@}

         /// @brief Get a hexdump of the serialized binary form of a transaction
         std::string get_transaction_hex(const signed_transaction& trx)const;

         /**
          *  @return the set of proposed transactions relevant to the specified account id.
          */
         vector<proposal_object> get_proposed_transactions( account_id_type id )const;

         /**
          *  @return all accounts that referr to the key or account id in their owner or active authorities.
          */
         vector<account_id_type> get_account_references( account_id_type account_id )const;
         vector<account_id_type> get_key_references( public_key_type account_id )const;

         /**
          *  @return all open margin positions for a given account id.
          */
         vector<call_order_object> get_margin_positions( const account_id_type& id )const;

         /** @return all unclaimed balance objects for a set of addresses */
         vector<balance_object>  get_balance_objects( const vector<address>& addrs )const;

      private:
         /** called every time a block is applied to report the objects that were changed */
         void on_objects_changed(const vector<object_id_type>& ids);
         void on_applied_block();

         fc::future<void>                                                                _broadcast_changes_complete;
         boost::signals2::scoped_connection                                              _change_connection;
         boost::signals2::scoped_connection                                              _applied_block_connection;
         map<object_id_type, std::function<void(const fc::variant&)> >                   _subscriptions;
         map< pair<asset_id_type,asset_id_type>, std::function<void(const variant&)> >  _market_subscriptions;
         graphene::chain::database&                                                      _db;
   };

   /**
    * @brief The history_api class implements the RPC API for account history
    *
    * This API contains methods to access account histories
    */
   class history_api
   {
      public:
         history_api(application& app):_app(app){}

         /**
          * @brief Get operations relevant to the specificed account
          * @param account The account whose history should be queried
          * @param stop ID of the earliest operation to retrieve
          * @param limit Maximum number of operations to retrieve (must not exceed 100)
          * @param start ID of the most recent operation to retrieve
          * @return A list of operations performed by account, ordered from most recent to oldest.
          */
         vector<operation_history_object> get_account_history(account_id_type account,
                                                              operation_history_id_type stop = operation_history_id_type(),
                                                              unsigned limit = 100,
                                                              operation_history_id_type start = operation_history_id_type())const;

         vector<bucket_object> get_market_history( asset_id_type a, asset_id_type b, uint32_t bucket_seconds, 
                                                   fc::time_point_sec start, fc::time_point_sec end )const;
         flat_set<uint32_t>    get_market_history_buckets()const;
      private:
           application&              _app;
   };

   /**
    * @brief The network_api class implements the RPC API for the network
    *
    * This API has methods to query the network status, connect to new peers, and send transactions.
    */
   class network_api
   {
      public:
         network_api(application& a);

         struct transaction_confirmation
         {
            transaction_id_type   id;
            uint32_t              block_num;
            uint32_t              trx_num;
            processed_transaction trx;
         };

         typedef std::function<void(variant/*transaction_confirmation*/)> confirmation_callback;

         /**
          * @brief Broadcast a transaction to the network
          * @param trx The transaction to broadcast
          *
          * The transaction will be checked for validity in the local database prior to broadcasting. If it fails to
          * apply locally, an error will be thrown and the transaction will not be broadcast.
          */
         void broadcast_transaction(const signed_transaction& trx);

         /** this version of broadcast transaction registers a callback method that will be called when the transaction is
          * included into a block.  The callback method includes the transaction id, block number, and transaction number in the
          * block.
          */
         void broadcast_transaction_with_callback( confirmation_callback cb, const signed_transaction& trx);

         /**
          * @brief add_node Connect to a new peer
          * @param ep The IP/Port of the peer to connect to
          */
         void add_node(const fc::ip::endpoint& ep);
         /**
          * @brief Get status of all current connections to peers
          */
         std::vector<net::peer_status> get_connected_peers() const;

         void on_applied_block( const signed_block& b );
      private:
         boost::signals2::scoped_connection             _applied_block_connection;
         map<transaction_id_type,confirmation_callback> _callbacks;
         application&                                   _app;
   };

   /**
    * @brief The login_api class implements the bottom layer of the RPC API
    *
    * All other APIs must be requested from this API.
    */
   class login_api
   {
      public:
         login_api(application& a);
         ~login_api();

         /**
          * @brief Authenticate to the RPC server
          * @param user Username to login with
          * @param password Password to login with
          * @return True if logged in successfully; false otherwise
          *
          * @note This must be called prior to requesting other APIs. Other APIs may not be accessible until the client
          * has sucessfully authenticated.
          */
         bool login(const string& user, const string& password);
         /// @brief Retrieve the network API
         fc::api<network_api> network()const;
         /// @brief Retrieve the database API
         fc::api<database_api> database()const;
         /// @brief Retrieve the history API
         fc::api<history_api> history()const;

      private:
         application&                      _app;
         optional< fc::api<database_api> > _database_api;
         optional< fc::api<network_api> >  _network_api;
         optional< fc::api<history_api> >  _history_api;
   };

}}  // graphene::app

FC_REFLECT( graphene::app::network_api::transaction_confirmation, 
        (id)(block_num)(trx_num)(trx) )

FC_API(graphene::app::database_api,
       (get_objects)
       (get_block_header)
       (get_block)
       (get_transaction)
       (get_global_properties)
       (get_dynamic_global_properties)
       (get_accounts)
       (get_assets)
       (lookup_account_names)
       (get_account_count)
       (lookup_accounts)
       (get_account_balances)
       (get_named_account_balances)
       (lookup_asset_symbols)
       (get_limit_orders)
       (get_call_orders)
       (get_settle_orders)
       (list_assets)
       (get_delegate_by_account)
       (get_witnesses)
       (get_delegates)
       (get_witness_by_account)
       (get_witness_count)
       (lookup_witness_accounts)
       (lookup_delegate_accounts)
       (subscribe_to_objects)
       (unsubscribe_from_objects)
       (subscribe_to_market)
       (unsubscribe_from_market)
       (cancel_all_subscriptions)
       (get_transaction_hex)
       (get_proposed_transactions)
       (get_account_references)
       (get_key_references)
       (get_margin_positions)
       (get_balance_objects)
     )
FC_API(graphene::app::history_api, (get_account_history)(get_market_history)(get_market_history_buckets))
FC_API(graphene::app::network_api, (broadcast_transaction)(broadcast_transaction_with_callback)
      /* (add_node)(get_connected_peers) */
       )
FC_API(graphene::app::login_api,
       (login)
       (network)
       (database)
       (history)
     )
