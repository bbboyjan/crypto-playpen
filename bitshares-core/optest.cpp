/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define BOOST_TEST_MODULE Test Application
#include <boost/test/included/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/witness_object.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( operation_tests, database_fixture )

//exec: /home/vm/bitshares-core/tests/optest
//workdir: /home/vm/bitshares-core/tests/


BOOST_AUTO_TEST_CASE( core_test_overbuy )
{ try {
      ACTORS((buyer)(seller)(borrower)(borrower2)(feedproducer));

      int64_t init_balance(1000000);
      transfer(committee_account, buyer_id, asset(init_balance));

      transfer(committee_account, seller_id, asset(init_balance));

      asset_id_type core_id = db.get_index<asset_object>().get_next_id();
      asset_create_operation creator;
      creator.issuer = account_id_type();  //buyer.get_id();
      creator.fee = asset();
      creator.symbol = "CORE";
      creator.common_options.max_supply = 100000000;
      creator.precision = 6;
      creator.common_options.market_fee_percent = GRAPHENE_MAX_MARKET_FEE_PERCENT/100; /*1%*/
      //creator.common_options.issuer_permissions = charge_market_fee|white_list|override_authority|transfer_restricted|disable_confidential;
      //creator.common_options.flags = charge_market_fee|white_list|override_authority|disable_confidential;
      creator.common_options.core_exchange_rate = price({asset(2),asset(1,asset_id_type(1))});
      //creator.common_options.whitelist_authorities.insert(buyer);
	  //creator.common_options.whitelist_authorities.insert(seller);
	  //creator.common_options.whitelist_authorities = creator.common_options.blacklist_authorities = {account_id_type()};
      trx.operations.push_back(std::move(creator));
      PUSH_TX( db, trx, ~0 );
	  trx.clear();

	  const asset_object& core = core_id(db);

      asset_id_type test_id = db.get_index<asset_object>().get_next_id();
      //asset_create_operation creator;
	  creator.issuer = account_id_type();  //buyer.get_id();
      creator.fee = asset();
      creator.symbol = "TEST";
      creator.common_options.max_supply = 100000000;
      creator.precision = 2;
      creator.common_options.market_fee_percent = GRAPHENE_MAX_MARKET_FEE_PERCENT/100; /*1%*/
      //creator1.common_options.issuer_permissions = charge_market_fee|white_list|override_authority|transfer_restricted|disable_confidential;
      //creator1.common_options.flags = charge_market_fee|white_list|override_authority|disable_confidential;
      creator.common_options.core_exchange_rate = price({asset(2),asset(1,asset_id_type(1))});
      //creator1.common_options.whitelist_authorities.insert(buyer);
	  //creator1.common_options.whitelist_authorities.insert(seller);
	  //creator1.common_options.whitelist_authorities = creator1.common_options.blacklist_authorities = {account_id_type()};
	  trx.operations.push_back(std::move(creator));
      PUSH_TX( db, trx, ~0 );
	  trx.clear();
	  
	  const asset_object& test = test_id(db);

      //const auto& bitusd = create_bitasset("USDBIT", feedproducer_id);
      //const auto& test  = asset_id_type()(db);
   } catch(fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}


BOOST_AUTO_TEST_CASE( create_uia )
{
   try {
      asset_id_type test_asset_id = db.get_index<asset_object>().get_next_id();
      asset_create_operation creator;
      creator.issuer = account_id_type();
      creator.fee = asset();
      creator.symbol = "TEST";
      creator.common_options.max_supply = 100000000;
      creator.precision = 2;
      creator.common_options.market_fee_percent = GRAPHENE_MAX_MARKET_FEE_PERCENT/100; /*1%*/
      creator.common_options.issuer_permissions = UIA_ASSET_ISSUER_PERMISSION_MASK;
      creator.common_options.flags = charge_market_fee;
      creator.common_options.core_exchange_rate = price({asset(2),asset(1,asset_id_type(1))});
      trx.operations.push_back(std::move(creator));
      PUSH_TX( db, trx, ~0 );

      const asset_object& test_asset = test_asset_id(db);
      BOOST_CHECK(test_asset.symbol == "TEST");
      BOOST_CHECK(asset(1, test_asset_id) * test_asset.options.core_exchange_rate == asset(2));
      BOOST_CHECK((test_asset.options.flags & white_list) == 0);
      BOOST_CHECK(test_asset.options.max_supply == 100000000);
      BOOST_CHECK(!test_asset.bitasset_data_id.valid());
      BOOST_CHECK(test_asset.options.market_fee_percent == GRAPHENE_MAX_MARKET_FEE_PERCENT/100);
      GRAPHENE_REQUIRE_THROW(PUSH_TX( db, trx, ~0 ), fc::exception);

      const asset_dynamic_data_object& test_asset_dynamic_data = test_asset.dynamic_asset_data_id(db);
      BOOST_CHECK(test_asset_dynamic_data.current_supply == 0);
      BOOST_CHECK(test_asset_dynamic_data.accumulated_fees == 0);
      BOOST_CHECK(test_asset_dynamic_data.fee_pool == 0);

      auto op = trx.operations.back().get<asset_create_operation>();
      op.symbol = "TESTFAIL";
      REQUIRE_THROW_WITH_VALUE(op, issuer, account_id_type(99999999));
      REQUIRE_THROW_WITH_VALUE(op, common_options.max_supply, -1);
      REQUIRE_THROW_WITH_VALUE(op, common_options.max_supply, 0);
      REQUIRE_THROW_WITH_VALUE(op, symbol, "A");
      REQUIRE_THROW_WITH_VALUE(op, symbol, "qqq");
      REQUIRE_THROW_WITH_VALUE(op, symbol, "11");
      REQUIRE_THROW_WITH_VALUE(op, symbol, ".AAA");
      REQUIRE_THROW_WITH_VALUE(op, symbol, "AAA.");
      REQUIRE_THROW_WITH_VALUE(op, symbol, "AB CD");
      REQUIRE_THROW_WITH_VALUE(op, symbol, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
      REQUIRE_THROW_WITH_VALUE(op, common_options.core_exchange_rate, price({asset(-100), asset(1)}));
      REQUIRE_THROW_WITH_VALUE(op, common_options.core_exchange_rate, price({asset(100),asset(-1)}));
   } catch(fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( create_account_test )
{
   try {
      trx.operations.push_back(make_account());
      account_create_operation op = trx.operations.back().get<account_create_operation>();

      REQUIRE_THROW_WITH_VALUE(op, registrar, account_id_type(9999999));
      REQUIRE_THROW_WITH_VALUE(op, fee, asset(-1));
      REQUIRE_THROW_WITH_VALUE(op, name, "!");
      REQUIRE_THROW_WITH_VALUE(op, name, "Sam");
      REQUIRE_THROW_WITH_VALUE(op, name, "saM");
      REQUIRE_THROW_WITH_VALUE(op, name, "sAm");
      REQUIRE_THROW_WITH_VALUE(op, name, "6j");
      REQUIRE_THROW_WITH_VALUE(op, name, "j-");
      REQUIRE_THROW_WITH_VALUE(op, name, "-j");
      REQUIRE_THROW_WITH_VALUE(op, name, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
      REQUIRE_THROW_WITH_VALUE(op, name, "aaaa.");
      REQUIRE_THROW_WITH_VALUE(op, name, ".aaaa");
      REQUIRE_THROW_WITH_VALUE(op, options.voting_account, account_id_type(999999999));

      auto auth_bak = op.owner;
      op.owner.add_authority(account_id_type(9999999999), 10);
      trx.operations.back() = op;
      op.owner = auth_bak;
      GRAPHENE_REQUIRE_THROW(PUSH_TX( db, trx, ~0 ), fc::exception);
      op.owner = auth_bak;

      trx.operations.back() = op;
      sign( trx,  init_account_priv_key );
      trx.validate();
      PUSH_TX( db, trx, ~0 );

      const account_object& nathan_account = *db.get_index_type<account_index>().indices().get<by_name>().find("nathan");
      BOOST_CHECK(nathan_account.id.space() == protocol_ids);
      BOOST_CHECK(nathan_account.id.type() == account_object_type);
      BOOST_CHECK(nathan_account.name == "nathan");

      BOOST_REQUIRE(nathan_account.owner.num_auths() == 1);
      BOOST_CHECK(nathan_account.owner.key_auths.at(committee_key) == 123);
      BOOST_REQUIRE(nathan_account.active.num_auths() == 1);
      BOOST_CHECK(nathan_account.active.key_auths.at(committee_key) == 321);
      BOOST_CHECK(nathan_account.options.voting_account == GRAPHENE_PROXY_TO_SELF_ACCOUNT);
      BOOST_CHECK(nathan_account.options.memo_key == committee_key);

      const account_statistics_object& statistics = nathan_account.statistics(db);
      BOOST_CHECK(statistics.id.space() == implementation_ids);
      BOOST_CHECK(statistics.id.type() == impl_account_statistics_object_type);
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( issue_uia )
{
   try {
      INVOKE(create_uia);
      INVOKE(create_account_test);

      const asset_object& test_asset = *db.get_index_type<asset_index>().indices().get<by_symbol>().find("TEST");
      const account_object& nathan_account = *db.get_index_type<account_index>().indices().get<by_name>().find("nathan");

      asset_issue_operation op;
      op.issuer = test_asset.issuer;
      op.asset_to_issue =  test_asset.amount(5000000);
      op.issue_to_account = nathan_account.id;
      trx.operations.push_back(op);

      REQUIRE_THROW_WITH_VALUE(op, asset_to_issue, asset(200));
      REQUIRE_THROW_WITH_VALUE(op, fee, asset(-1));
      REQUIRE_THROW_WITH_VALUE(op, issue_to_account, account_id_type(999999999));

      trx.operations.back() = op;
      PUSH_TX( db, trx, ~0 );

      const asset_dynamic_data_object& test_dynamic_data = test_asset.dynamic_asset_data_id(db);
      BOOST_CHECK_EQUAL(get_balance(nathan_account, test_asset), 5000000);
      BOOST_CHECK(test_dynamic_data.current_supply == 5000000);
      BOOST_CHECK(test_dynamic_data.accumulated_fees == 0);
      BOOST_CHECK(test_dynamic_data.fee_pool == 0);

      PUSH_TX( db, trx, ~0 );

      BOOST_CHECK_EQUAL(get_balance(nathan_account, test_asset), 10000000);
      BOOST_CHECK(test_dynamic_data.current_supply == 10000000);
      BOOST_CHECK(test_dynamic_data.accumulated_fees == 0);
      BOOST_CHECK(test_dynamic_data.fee_pool == 0);
   } catch(fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

/*const limit_order_object* database_fixture::create_sell_order_with_flag( const account_object& user, const asset& amount, const asset& recv, bool above1 )
{
   //wdump((amount)(recv));
   limit_order_create_operation buy_order;
   buy_order.seller = user.id;
   buy_order.amount_to_sell = amount;
   buy_order.min_to_receive = recv;
   if (above1) buy_order.expiration = buy_order.expiration.maximum()-1000; else 
		buy_order.expiration = buy_order.expiration.maximum()-1001;
   trx.operations.push_back(buy_order);
   for( auto& op : trx.operations ) db.current_fee_schedule().set_fee(op);
   trx.validate();
   auto processed = db.push_transaction(trx, ~0);
   trx.operations.clear();
   verify_asset_supplies(db);
   //wdump((processed));
   return db.find<limit_order_object>( processed.operation_results[0].get<object_id_type>() );
}*/

const limit_order_object* create_sell_order_with_ext( database& db, signed_transaction& trx, const account_object& user, const asset& amount, const asset& recv, bool isCoreBuySell )
{
   //wdump((amount)(recv));
   limit_order_create_operation buy_order;
   buy_order.seller = user.id;
   buy_order.amount_to_sell = amount;
   buy_order.min_to_receive = recv;

	graphene::chain::limit_order_create_operation::limit_order_flags order_flags;
	order_flags.isCoreBuySell = isCoreBuySell;
	extension<graphene::chain::limit_order_create_operation::limit_order_flags> ext;
	ext.value = order_flags;
	buy_order.extensions = ext;

   trx.operations.push_back(buy_order);
   for( auto& op : trx.operations ) db.current_fee_schedule().set_fee(op);
   trx.validate();
   auto processed = db.push_transaction(trx, ~0);
   trx.operations.clear();
   database_fixture::verify_asset_supplies(db);
   //wdump((processed));
   return db.find<limit_order_object>( processed.operation_results[0].get<object_id_type>() );
}

void matching_test_case_sell_buy(const asset_object&   core_asset, const asset_object&   test_asset,
 const account_object& buyer_account,
 const account_object& seller_account,
 int test_sell, int core_sell, int core_buy, int test_buy) {
    asset usd_pays, usd_receives, core_pays, core_receives;
	price match_price;
	limit_order_object ask_core, bid_usd;
	fc::safe<long int> usd_max_counter_size = 0;
	fc::safe<long int> pay_refund = 0;
	ask_core = limit_order_object();
      ask_core.seller = seller_account.get_id();
	  ask_core.sell_price = price(test_asset.amount(test_sell/*80*/), core_asset.amount(core_sell/*480*/));  //sell 6
	  ask_core.for_sale = test_sell/*80*/;	
	bid_usd = limit_order_object();
	  bid_usd.seller = buyer_account.get_id();
	  bid_usd.sell_price = price(core_asset.amount(core_buy/*21*/), test_asset.amount(test_buy/*2*/));  //buy 10.5
	  bid_usd.for_sale = core_buy/*21*/;
	match_price = ask_core.sell_price;
	idump((1/ask_core.sell_price.to_real()));
	idump((bid_usd.sell_price.to_real()));
	idump((bid_usd.amount_for_sale() > match_price));
	usd_max_counter_size = 0;
	usd_max_counter_size = (bid_usd.amount_to_receive() * match_price).amount.value;
	idump((usd_max_counter_size));
	idump((bid_usd.amount_for_sale()));
	pay_refund = bid_usd.amount_for_sale().amount.value - usd_max_counter_size;
	idump((pay_refund));
}

void matching_test_case_buy_sell(const asset_object&   core_asset, const asset_object&   test_asset,
 const account_object& buyer_account, const account_object& seller_account, int core_buy, int test_buy, int test_sell, int core_sell) {
    asset usd_pays, usd_receives, core_pays, core_receives;
	price match_price;
	limit_order_object ask_core, bid_usd;
	fc::safe<long int> usd_max_counter_size = 0;
	fc::safe<long int> pay_refund = 0;
	ask_core = limit_order_object();
      ask_core.seller = seller_account.get_id();
	  ask_core.sell_price = price(core_asset.amount(core_buy/*21*/), test_asset.amount(test_buy/*2*/));  //buy 10.5
	  ask_core.for_sale = core_buy/*21*/;
	bid_usd = limit_order_object();
	  bid_usd.seller = buyer_account.get_id();
	  bid_usd.sell_price = price(test_asset.amount(test_sell/*80*/), core_asset.amount(core_sell/*480*/));  //sell 6
	  bid_usd.for_sale = test_sell/*80*/;	
	match_price = ask_core.sell_price;
	idump((ask_core.sell_price.to_real()));
	idump((1/bid_usd.sell_price.to_real()));
	idump((bid_usd.amount_for_sale() > match_price));
	usd_max_counter_size = 0;
	usd_max_counter_size = (bid_usd.amount_to_receive() * match_price).amount.value;
	idump((usd_max_counter_size));
	idump((bid_usd.amount_for_sale()));
	pay_refund = bid_usd.amount_for_sale().amount.value - usd_max_counter_size;
	idump((pay_refund));
}


BOOST_AUTO_TEST_CASE( rounded_match_test_cases )
{ try {
   INVOKE( issue_uia );
    const asset_object&   test_asset     = get_asset( "TEST" );
    const asset_object&   core_asset     = get_asset( GRAPHENE_SYMBOL );
    const account_object& nathan_account = get_account( "nathan" );
    const account_object& buyer_account  = create_account( "buyer" );
    const account_object& seller_account = create_account( "seller" );

   transfer( committee_account(db), buyer_account, core_asset.amount( 100000 ) );
   transfer( nathan_account, seller_account, test_asset.amount(100000) );
   //auto order = create_sell_order_with_ext( db, trx, seller_account, core_asset.amount(15000), test_asset.amount(1000), true );

	/*struct order_ext {
	  optional<bool> isCoreBuySell;
	  //void validate()const {}
	};
   extension<order_ext> ext;
   ext.value.isCoreBuySell = true;
   limit_order_create_operation op;
   //op.extensions.insert(ext);
   op.extensions = ext;*/
   /*	extension<graphene::chain::limit_order_create_operation::order_ext> ext;
    limit_order_create_operation op;
	bool_order_flags order_flags;
	order_flags.isCoreBuySell = +1;
	sv_order_flags sv_flags;
	sv_flags = order_flags;
	idump((sv_flags));
	ext.value.order_flags = sv_flags;
	op.extensions = ext;
	idump((op.extensions));
	idump((op.extensions.value.order_flags));
	fc::optional<fc::static_variant<graphene::chain::bool_order_flags>> get_ext = op.extensions.value.order_flags;
	idump((get_ext));*/

    limit_order_create_operation op;
	graphene::chain::limit_order_create_operation::limit_order_flags order_flags;
	order_flags.isCoreBuySell = false;
	extension<graphene::chain::limit_order_create_operation::limit_order_flags> ext;
	ext.value = order_flags;
	op.extensions = ext;
	idump((op.extensions));
	graphene::chain::limit_order_create_operation::limit_order_flags get_order_flags;
	get_order_flags = op.extensions.value;
	idump((get_order_flags));
	idump((get_order_flags.isCoreBuySell));
	if (get_order_flags.isCoreBuySell.valid()) {
		idump((get_order_flags.isCoreBuySell));
		if (*get_order_flags.isCoreBuySell == false) ilog("orderFlagFalse"); else ilog("orderFlagTrue");
	}
	
	//idump((*get_ext->isCoreBuySell));
	//get_ext = [0,{"isCoreBuySell":1}]
	//idump(( get_ext.isCoreBuySell ));  //  <-- error (no member isCoreBuySell)
	//idump(( (bool_order_type)op.extensions.value.order_flags->isCoreBuySell ));   //  <-- error (no member order_flag)

	//sv_bool<bool_order_type> get_ext = op.extensions;
	//bool_order_type get_flags = (bool_order_type)get_ext;
	//idump(( get_flags.order_flag ));
	
	//idump(( (bool_order_type)((sv_bool)op.extensions.value.isCoreBuySell).order_flag ));
	//op.extensions.value.isCoreBuySell->order_flag = true;
	//op.extensions = ext;

	matching_test_case_sell_buy(core_asset,test_asset,buyer_account,seller_account,500*100,1000*100000,5000*100000,1000*100);
	matching_test_case_buy_sell(core_asset,test_asset,buyer_account,seller_account,21,2,80,480);

	matching_test_case_sell_buy(core_asset,test_asset,buyer_account,seller_account,80,480,21,2);
	matching_test_case_buy_sell(core_asset,test_asset,buyer_account,seller_account,21,2,80,480);

	matching_test_case_buy_sell(core_asset,test_asset,buyer_account,seller_account,150,100,11,5);
	matching_test_case_sell_buy(core_asset,test_asset,buyer_account,seller_account,100,80,15,11);

	matching_test_case_buy_sell(core_asset,test_asset,buyer_account,seller_account,400,100,300,600);
	matching_test_case_buy_sell(core_asset,test_asset,buyer_account,seller_account,50,100,300,50);

	matching_test_case_sell_buy(core_asset,test_asset,buyer_account,seller_account,100,25,275,300);
	matching_test_case_sell_buy(core_asset,test_asset,buyer_account,seller_account,100,300,1500,300);


      asset usd_pays, usd_receives, core_pays, core_receives;
	  price match_price;
	  //price ask_core = price(asset(80,whalehole_id), asset(480,core_id));
	  //price bid_usd = price(asset(21,core_id), asset(2,whalehole_id));	

	limit_order_object ask_core, bid_usd;
	fc::safe<long int> usd_max_counter_size = 0;
	fc::safe<long int> pay_refund = 0;
	
	// bid-taker
	ask_core = limit_order_object();
      ask_core.seller = seller_account.get_id();
	  ask_core.sell_price = price(test_asset.amount(80), core_asset.amount(480));  //6
	  ask_core.for_sale = 80;	
	bid_usd = limit_order_object();
	  bid_usd.seller = buyer_account.get_id();
	  bid_usd.sell_price = price(core_asset.amount(21), test_asset.amount(2));  //10.5
	  bid_usd.for_sale = 21;
	match_price = ask_core.sell_price;
	idump((bid_usd.amount_for_sale() > match_price));
	usd_max_counter_size = 0;
	usd_max_counter_size = (bid_usd.amount_to_receive() * match_price).amount.value;
	idump((usd_max_counter_size));
	idump((bid_usd.amount_for_sale()));
	pay_refund = bid_usd.amount_for_sale().amount.value - usd_max_counter_size;
	idump((pay_refund));


 }
 catch ( const fc::exception& e )
 {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
 }
}

BOOST_AUTO_TEST_CASE( taker_sells_overpays_huge_lot_isTestBuySell )
{ try {
   INVOKE( issue_uia );
    const asset_object&   test_asset     = get_asset( "TEST" );
    const asset_object&   core_asset     = get_asset( GRAPHENE_SYMBOL );
    const account_object& nathan_account = get_account( "nathan" );
    const account_object& buyer_account  = create_account( "buyer" );
    const account_object& seller_account = create_account( "seller" );

   transfer( committee_account(db), buyer_account, core_asset.amount( 100000 ) );
   transfer( nathan_account, seller_account, test_asset.amount(100000) );

   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 100000 );

   limit_order_id_type first_id  = create_sell_order( buyer_account, core_asset.amount(15000), test_asset.amount(1000) )->id;

   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 85000 );

   //print_market( "", "" );
   auto unmatched = create_sell_order_with_ext( db, trx, seller_account, test_asset.amount(5000), core_asset.amount(100) , false );
   //print_market( "", "" );
   BOOST_CHECK( !db.find( first_id ) );
   if( unmatched ) wdump((*unmatched));
   BOOST_CHECK( unmatched );

   //APM
   //sell_asset nathan 15000 BTS 1000 TEST 100000 false true    <-- buyer BUY 1000 TEST @ 15.00 (bts)
   //sell_asset nathan 5000 TEST 100 BTS 100000 false true    <-- seller SELL 5000 TEST @ 0.02
   //expected result: 1000 TEST filled @15.00, remainder: 4000 TEST @ 0.02
   // buyer is buying TEST selling CORE
   // seller is selling TEST buying CORE
   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 95000 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 85000 );
   BOOST_CHECK_EQUAL( test_asset.dynamic_asset_data_id(db).accumulated_fees.value , 10 );
   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 15000 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 990 );
 }
 catch ( const fc::exception& e )
 {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
 }
}

BOOST_AUTO_TEST_CASE( taker_buys_overpays_huge_lot_isTestBuySell )
{ try {
   INVOKE( issue_uia );
    const asset_object&   test_asset     = get_asset( "TEST" );
    const asset_object&   core_asset     = get_asset( GRAPHENE_SYMBOL );
    const account_object& nathan_account = get_account( "nathan" );
    const account_object& buyer_account  = create_account( "buyer" );
    const account_object& seller_account = create_account( "seller" );

   transfer( committee_account(db), buyer_account, core_asset.amount( 100000 ) );
   transfer( nathan_account, seller_account, test_asset.amount(100000) );

   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 100000 );

   limit_order_id_type first_id  = create_sell_order( seller_account, test_asset.amount(1000), core_asset.amount(4000) )->id;
   limit_order_id_type second_id = create_sell_order( seller_account, test_asset.amount(1000), core_asset.amount(3000) )->id;

   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 98000 );

   //print_market( "", "" );
   auto unmatched = create_sell_order_with_ext( db, trx, buyer_account, core_asset.amount(8000), test_asset.amount(200), false );
   //print_market( "", "" );
   BOOST_CHECK( db.find( first_id ) );
   BOOST_CHECK( db.find( second_id ) );
   if( unmatched ) wdump((*unmatched));
   BOOST_CHECK( !unmatched );

   //APM
   //sell_asset nathan 1000 TEST 4000 BTS 100000 false true    <-- seller SELL 1000 TEST @ 4 (bts)
   //sell_asset nathan 1000 TEST 3000 BTS 100000 false true    <-- seller SELL 1000 TEST @ 3
   //sell_asset nathan 8000 BTS 200 TEST 100000 false true    <-- buyer BUY 200 TEST @ 40
   //expected result: 200 TEST filled @3
   // seller is selling TEST buying CORE
   // buyer is buying TEST selling CORE
   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 98000 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 99400 );
   BOOST_CHECK_EQUAL( test_asset.dynamic_asset_data_id(db).accumulated_fees.value , 2);
   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 600 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 198 );
 }
 catch ( const fc::exception& e )
 {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
 }
}

BOOST_AUTO_TEST_CASE( taker_sells_overpays_huge_lot_isCoreBuySell )
{ try {
   INVOKE( issue_uia );
    const asset_object&   test_asset     = get_asset( "TEST" );
    const asset_object&   core_asset     = get_asset( GRAPHENE_SYMBOL );
    const account_object& nathan_account = get_account( "nathan" );
    const account_object& buyer_account  = create_account( "buyer" );
    const account_object& seller_account = create_account( "seller" );

   transfer( committee_account(db), buyer_account, core_asset.amount( 100000 ) );
   transfer( nathan_account, seller_account, test_asset.amount(100000) );

   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 100000 );

   limit_order_id_type first_id  = create_sell_order( buyer_account, core_asset.amount(15000), test_asset.amount(1000) )->id;

   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 85000 );

   //print_market( "", "" );
   auto unmatched = create_sell_order_with_ext( db, trx, seller_account, test_asset.amount(5000), core_asset.amount(100) , true );
   //print_market( "", "" );
   BOOST_CHECK( db.find( first_id ) );
   if( unmatched ) wdump((*unmatched));
   BOOST_CHECK( !unmatched );

   //APM
   //sell_asset nathan 15000 BTS 1000 TEST 100000 false true    <-- buyer BUY 1000 TEST @ 15.00 (bts)
   //sell_asset nathan 5000 TEST 100 BTS 100000 false true    <-- seller SELL 5000 TEST @ 0.02
   //expected result: 90 TEST filled @15.00, remainder: canceled
   // buyer is buying TEST selling CORE
   // seller is selling TEST buying CORE
   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 99994 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 85000 );
   BOOST_CHECK_EQUAL( core_asset.dynamic_asset_data_id(db).accumulated_fees.value , 0 );
   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 90 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 6 );
 }
 catch ( const fc::exception& e )
 {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
 }
}

BOOST_AUTO_TEST_CASE( taker_buys_overpays_huge_lot_isCoreBuySell )
{ try {
   INVOKE( issue_uia );
    const asset_object&   test_asset     = get_asset( "TEST" );
    const asset_object&   core_asset     = get_asset( GRAPHENE_SYMBOL );
    const account_object& nathan_account = get_account( "nathan" );
    const account_object& buyer_account  = create_account( "buyer" );
    const account_object& seller_account = create_account( "seller" );

   transfer( committee_account(db), seller_account, core_asset.amount( 100000 ) );
   transfer( nathan_account, buyer_account, test_asset.amount(100000) );

   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 100000 );

   limit_order_id_type first_id  = create_sell_order( seller_account, core_asset.amount(1000), test_asset.amount(4000) )->id;
   limit_order_id_type second_id = create_sell_order( seller_account, core_asset.amount(1000), test_asset.amount(3000) )->id;

   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 98000 );

   //print_market( "", "" );
   auto unmatched = create_sell_order_with_ext( db, trx, buyer_account, test_asset.amount(8000), core_asset.amount(200), true );
   //print_market( "", "" );
   BOOST_CHECK( db.find( first_id ) );
   BOOST_CHECK( db.find( second_id ) );
   if( unmatched ) wdump((*unmatched));
   BOOST_CHECK( !unmatched );

   //APM
   //sell_asset nathan 1000 CORE 4000 TEST 100000 false true    <-- seller SELL 4000 CORE @ 4 (test)
   //sell_asset nathan 1000 CORE 3000 TEST 100000 false true    <-- seller SELL 3000 CORE @ 3
   //sell_asset nathan 8000 TEST 200 CORE 100000 false true    <-- buyer BUY 200 CORE @ 40
   //expected result: 200 CORE filled @3
   // seller is selling CORE buying TEST
   // buyer is buying CORE selling TEST
   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 594 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 200 );
   BOOST_CHECK_EQUAL( test_asset.dynamic_asset_data_id(db).accumulated_fees.value , 6);
   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 98000 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 99400 );
 }
 catch ( const fc::exception& e )
 {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
 }
}

BOOST_AUTO_TEST_CASE( taker_buys_overpays_huge_lot_isCoreBuySell_2 )
{ try {
   INVOKE( issue_uia );
    const asset_object&   test_asset     = get_asset( "TEST" );
    const asset_object&   core_asset     = get_asset( GRAPHENE_SYMBOL );
    const account_object& nathan_account = get_account( "nathan" );
    const account_object& buyer_account  = create_account( "buyer" );
    const account_object& seller_account = create_account( "seller" );

   transfer( committee_account(db), seller_account, core_asset.amount( 100000 ) );
   transfer( nathan_account, buyer_account, test_asset.amount(100000) );

   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 100000 );

   limit_order_id_type first_id  = create_sell_order( seller_account, core_asset.amount(1000), test_asset.amount(4000) )->id;
   limit_order_id_type second_id = create_sell_order( seller_account, core_asset.amount(10000), test_asset.amount(30000) )->id;

   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 89000 );

   //print_market( "", "" );
   auto unmatched = create_sell_order_with_ext( db, trx, buyer_account, test_asset.amount(1000), core_asset.amount(20), true );
   //print_market( "", "" );
   BOOST_CHECK( db.find( first_id ) );
   BOOST_CHECK( db.find( second_id ) );
   if( unmatched ) wdump((*unmatched));
   BOOST_CHECK( !unmatched );

   //APM
   //sell_asset nathan 1000 BTS 4000 TEST 100000 false true    <-- seller SELL 1000 BTS @ 4 (test)
   //sell_asset nathan 10000 BTS 30000 TEST 100000 false true    <-- seller SELL 10000 BTS @ 3
   //sell_asset nathan 1000 TEST 20 BTS 100000 false true    <-- buyer BUY 20 BTS @ 50
   //expected result: 20 BTS filled @ 3.0000
   // seller is selling TEST buying CORE
   // buyer is buying TEST selling CORE
   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 60 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 20 );
   BOOST_CHECK_EQUAL( core_asset.dynamic_asset_data_id(db).accumulated_fees.value , 0);
   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 89000 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 99940 );
 }
 catch ( const fc::exception& e )
 {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
 }
}

BOOST_AUTO_TEST_CASE( taker_buys_overpays_vs_huge_lot_check_satoshi_rounding_isTestBuySell )
{ try {
   INVOKE( issue_uia );
    const asset_object&   test_asset     = get_asset( "TEST" );
    const asset_object&   core_asset     = get_asset( GRAPHENE_SYMBOL );
    const account_object& nathan_account = get_account( "nathan" );
    const account_object& buyer_account  = create_account( "buyer" );
    const account_object& seller_account = create_account( "seller" );

   transfer( committee_account(db), seller_account, core_asset.amount( 100000 ) );
   transfer( nathan_account, buyer_account, test_asset.amount(100000) );

   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 100000 );

   limit_order_id_type first_id = create_sell_order( seller_account, core_asset.amount(10234), test_asset.amount(35323) )->id;

   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 89766 );

   //print_market( "", "" );
   auto unmatched = create_sell_order_with_ext( db, trx, buyer_account, test_asset.amount(429), core_asset.amount(123), false );
   //print_market( "", "" );
   BOOST_CHECK( db.find( first_id ) );
   if( unmatched ) wdump((*unmatched));
   BOOST_CHECK( !unmatched );

   //APM
   //sell_asset nathan 10234 BTS 35323 TEST 100000 false true    <-- seller SELL 10234 BTS @ 3.45153 (test)
   //sell_asset nathan 429 TEST 123 BTS 100000 false true    <-- buyer BUY 20 BTS @ 3.4878
   //expected result: 129 TEST filled @3.45153
   // seller is selling TEST buying CORE
   // buyer is buying TEST selling CORE
   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 425 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 124 );
   BOOST_CHECK_EQUAL( test_asset.dynamic_asset_data_id(db).accumulated_fees.value , 4);
   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 89766 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 99571 );
 }
 catch ( const fc::exception& e )
 {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
 }
}

BOOST_AUTO_TEST_CASE( taker_sells_through_1to1_bid )
{ try {
   INVOKE( issue_uia );
    const asset_object&   test_asset     = get_asset( "TEST" );
    const asset_object&   core_asset     = get_asset( GRAPHENE_SYMBOL );
    const account_object& nathan_account = get_account( "nathan" );
    const account_object& buyer_account  = create_account( "buyer" );
    const account_object& seller_account = create_account( "seller" );

   transfer( committee_account(db), buyer_account, core_asset.amount( 10000 ) );
   transfer( nathan_account, seller_account, test_asset.amount(10000) );

   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 10000 );

   limit_order_id_type first_id  = create_sell_order( buyer_account, core_asset.amount(50), test_asset.amount(100) )->id;
   limit_order_id_type second_id = create_sell_order( buyer_account, core_asset.amount(100), test_asset.amount(100) )->id;

   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 9850 );

   //print_market( "", "" );
   auto unmatched = create_sell_order_with_ext( db, trx, seller_account, test_asset.amount(100), core_asset.amount(90), false );
   //print_market( "", "" );
   BOOST_CHECK( db.find( first_id ) );
   BOOST_CHECK( !db.find( second_id ) );
   if( unmatched ) wdump((*unmatched));
   BOOST_CHECK( !unmatched );

   //APM
   //sell_asset nathan 50 BTS 100 TEST 100000 false true    <-- buyer BUY 100 TEST @ 0.5 (bts)
   //sell_asset nathan 100 BTS 100 TEST 100000 false true    <-- buyer BUY 100 TEST @ 1
   //sell_asset nathan 100 TEST 90 BTS 100000 false true    <-- seller SELL 100 TEST @ 0.909090
   //expected result: 100 TEST filled @1
   // buyer is buying TEST selling CORE
   // seller is selling TEST buying CORE
   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 9900 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 9850 );
   BOOST_CHECK_EQUAL( test_asset.dynamic_asset_data_id(db).accumulated_fees.value , 1 );
   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 100 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 99 );
 }
 catch ( const fc::exception& e )
 {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
 }
}

BOOST_AUTO_TEST_CASE( taker_sells_small_lot_too_low_through_1to1 )
{ try {
   INVOKE( issue_uia );
    const asset_object&   test_asset     = get_asset( "TEST" );
    const asset_object&   core_asset     = get_asset( GRAPHENE_SYMBOL );
    const account_object& nathan_account = get_account( "nathan" );
    const account_object& buyer_account  = create_account( "buyer" );
    const account_object& seller_account = create_account( "seller" );

   transfer( committee_account(db), buyer_account, core_asset.amount( 10000 ) );
   transfer( nathan_account, seller_account, test_asset.amount(10000) );

   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 10000 );

   limit_order_id_type first_id  = create_sell_order( buyer_account, core_asset.amount(150), test_asset.amount(100) )->id;

   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 9850 );

   //print_market( "", "" );
   auto unmatched = create_sell_order_with_ext( db, trx, seller_account, test_asset.amount(11), core_asset.amount(5), false );
   //print_market( "", "" );
   BOOST_CHECK( db.find( first_id ) );
   if( unmatched ) wdump((*unmatched));
   BOOST_CHECK( !unmatched );

   //APM
   //sell_asset nathan 150 BTS 100 TEST 100000 false true    <-- buyer BUY 100 TEST @ 1.50 (bts)
   //sell_asset nathan 11 TEST 5 BTS 100000 false true    <-- seller SELL 11 TEST @ 0.454545
   //expected result: 11 TEST filled @1.5
   // buyer is buying TEST selling CORE
   // seller is selling TEST buying CORE
   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 9989 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 9850 );
   BOOST_CHECK_EQUAL( test_asset.dynamic_asset_data_id(db).accumulated_fees.value , 0 );
   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 16 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 11 );
 }
 catch ( const fc::exception& e )
 {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
 }
}

BOOST_AUTO_TEST_CASE( taker_buys_small_lot_too_high_through_1to1 )
{ try {
   INVOKE( issue_uia );
    const asset_object&   test_asset     = get_asset( "TEST" );
    const asset_object&   core_asset     = get_asset( GRAPHENE_SYMBOL );
    const account_object& nathan_account = get_account( "nathan" );
    const account_object& buyer_account  = create_account( "buyer" );
    const account_object& seller_account = create_account( "seller" );

   transfer( committee_account(db), buyer_account, core_asset.amount( 10000 ) );
   transfer( nathan_account, seller_account, test_asset.amount(10000) );

   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 10000 );

   limit_order_id_type first_id  = create_sell_order( seller_account, test_asset.amount(100), core_asset.amount(80) )->id;

   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 9900 );

   //print_market( "", "" );
   auto unmatched = create_sell_order_with_ext( db, trx, buyer_account, core_asset.amount(15), test_asset.amount(11), false );
   //print_market( "", "" );
   BOOST_CHECK( db.find( first_id ) );
   if( unmatched ) wdump((*unmatched));
   BOOST_CHECK( !unmatched );

   //APM
   //sell_asset nathan 100 TEST 80 BTS 100000 false true    <-- seller SELL 100 TEST @ 0.80 (bts)
   //sell_asset nathan 15 CORE 11 TEST 100000 false true    <-- buyer BUY 11 TEST @ 1.363636
   //expected result: 10 TEST filled @0.80, remainder: canceled
   // seller is selling TEST buying CORE
   // buyer is buying TEST selling CORE
   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 9900 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 9992 );
   BOOST_CHECK_EQUAL( core_asset.dynamic_asset_data_id(db).accumulated_fees.value , 0 );
   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 8 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 10 );
 }
 catch ( const fc::exception& e )
 {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
 }
}

BOOST_AUTO_TEST_CASE( taker_sells_above_1 )
{ try {
   INVOKE( issue_uia );
    const asset_object&   test_asset     = get_asset( "TEST" );
    const asset_object&   core_asset     = get_asset( GRAPHENE_SYMBOL );
    const account_object& nathan_account = get_account( "nathan" );
    const account_object& buyer_account  = create_account( "buyer" );
    const account_object& seller_account = create_account( "seller" );

   transfer( committee_account(db), buyer_account, core_asset.amount( 10000 ) );
   transfer( nathan_account, seller_account, test_asset.amount(10000) );

   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 10000 );

   limit_order_id_type first_id  = create_sell_order( buyer_account, core_asset.amount(400), test_asset.amount(100) )->id;
   limit_order_id_type second_id = create_sell_order( buyer_account, core_asset.amount(300), test_asset.amount(100) )->id;

   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 9300 );

   //print_market( "", "" );
   auto unmatched = create_sell_order_with_ext( db, trx, seller_account, test_asset.amount(300), core_asset.amount(600), false );
   //print_market( "", "" );
   BOOST_CHECK( !db.find( first_id ) );
   BOOST_CHECK( !db.find( second_id ) );
   if( unmatched ) wdump((*unmatched));
   BOOST_CHECK( unmatched );

   //APM
   //sell_asset nathan 400 BTS 100 TEST 100000 false true    <-- buyer BUY 100 TEST @ 4 (bts)
   //sell_asset nathan 300 BTS 100 TEST 100000 false true    <-- buyer BUY 100 TEST @ 3
   //sell_asset nathan 300 TEST 600 BTS 100000 false true    <-- seller SELL 300 TEST @ 2
   //expected result: 100 TEST filled @4, 100 TEST filled @3, remainder: 100 TEST offered @2
   // buyer is buying TEST selling CORE
   // seller is selling TEST buying CORE
   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 9700 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 9300 );
   BOOST_CHECK_EQUAL( test_asset.dynamic_asset_data_id(db).accumulated_fees.value , 2 );
   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 700 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 198 );
 }
 catch ( const fc::exception& e )
 {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
 }
}



BOOST_AUTO_TEST_CASE( taker_sells_below_1 )
{ try {
   INVOKE( issue_uia );
    const asset_object&   test_asset     = get_asset( "TEST" );
    const asset_object&   core_asset     = get_asset( GRAPHENE_SYMBOL );
    const account_object& nathan_account = get_account( "nathan" );
    const account_object& buyer_account  = create_account( "buyer" );
    const account_object& seller_account = create_account( "seller" );

   transfer( committee_account(db), buyer_account, core_asset.amount( 10000 ) );
   transfer( nathan_account, seller_account, test_asset.amount(10000) );

   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 10000 );

   limit_order_id_type first_id  = create_sell_order( buyer_account, core_asset.amount(25), test_asset.amount(100) )->id;
   limit_order_id_type second_id = create_sell_order( buyer_account, core_asset.amount(50), test_asset.amount(100) )->id;

   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 9925 );

   //print_market( "", "" );
   auto unmatched = create_sell_order_with_ext( db, trx, seller_account, test_asset.amount(300), core_asset.amount(50), false );
   //print_market( "", "" );
   BOOST_CHECK( !db.find( first_id ) );
   BOOST_CHECK( !db.find( second_id ) );
   if( unmatched ) wdump((*unmatched));
   BOOST_CHECK( unmatched );

   //APM
   //sell_asset nathan 25 BTS 100 TEST 100000 false true    <-- buyer BUY 100 TEST @ 0.25 (bts)
   //sell_asset nathan 50 BTS 100 TEST 100000 false true    <-- buyer BUY 100 TEST @ 0.50
   //sell_asset nathan 300 TEST 50 BTS 100000 false true    <-- seller SELL 300 TEST @0.16667
   //expected result: 100 TEST filled @0.50, 100 TEST filled @0.25, remainder: 100 TEST offered @0.16667
   // buyer is buying TEST selling CORE
   // seller is selling TEST buying CORE
   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 9700 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 9925 );
   BOOST_CHECK_EQUAL( test_asset.dynamic_asset_data_id(db).accumulated_fees.value , 2 );
   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 75 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 198 );
 }
 catch ( const fc::exception& e )
 {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
 }
}


BOOST_AUTO_TEST_CASE( taker_buys_below_1 )
{ try {
   INVOKE( issue_uia );
    const asset_object&   test_asset     = get_asset( "TEST" );
    const asset_object&   core_asset     = get_asset( GRAPHENE_SYMBOL );
    const account_object& nathan_account = get_account( "nathan" );
    const account_object& buyer_account  = create_account( "buyer" );
    const account_object& seller_account = create_account( "seller" );

   transfer( committee_account(db), buyer_account, core_asset.amount( 10000 ) );
   transfer( nathan_account, seller_account, test_asset.amount(10000) );

   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 10000 );

   limit_order_id_type first_id  = create_sell_order( seller_account, test_asset.amount(100), core_asset.amount(25) )->id;
   limit_order_id_type second_id = create_sell_order( seller_account, test_asset.amount(100), core_asset.amount(50) )->id;

   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 9800 );

   //print_market( "", "" );
   auto unmatched = create_sell_order_with_ext( db, trx, buyer_account, core_asset.amount(275), test_asset.amount(300), false );
   //print_market( "", "" );
   BOOST_CHECK( !db.find( first_id ) );
   BOOST_CHECK( !db.find( second_id ) );
   if( unmatched ) wdump((*unmatched));
   BOOST_CHECK( unmatched );

   //APM
   //sell_asset nathan 100 TEST 25 BTS 100000 false true    <-- buyer SELL 100 TEST @ 0.25 (bts)
   //sell_asset nathan 100 TEST 50 BTS 100000 false true    <-- buyer SELL 100 TEST @ 0.50
   //sell_asset nathan 275 BTS 300 TEST 100000 false true    <-- seller BUY 275 TEST @0.916667
   //expected result: 100 TEST filled @0.25, 100 TEST filled @0.50, remainder: 100 TEST bid @0.916667
   // buyer is selling TEST buying CORE
   // seller is buying TEST selling CORE
   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 9800 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 9832 );
   BOOST_CHECK_EQUAL( test_asset.dynamic_asset_data_id(db).accumulated_fees.value , 2 );
   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 75 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 198 );
 }
 catch ( const fc::exception& e )
 {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
 }
}

BOOST_AUTO_TEST_CASE( taker_buys_above_1 )
{ try {
   INVOKE( issue_uia );
    const asset_object&   test_asset     = get_asset( "TEST" );
    const asset_object&   core_asset     = get_asset( GRAPHENE_SYMBOL );
    const account_object& nathan_account = get_account( "nathan" );
    const account_object& buyer_account  = create_account( "buyer" );
    const account_object& seller_account = create_account( "seller" );

   transfer( committee_account(db), buyer_account, core_asset.amount( 10000 ) );
   transfer( nathan_account, seller_account, test_asset.amount(10000) );

   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 10000 );

   limit_order_id_type first_id  = create_sell_order( seller_account, test_asset.amount(100), core_asset.amount(400) )->id;
   limit_order_id_type second_id = create_sell_order( seller_account, test_asset.amount(100), core_asset.amount(300) )->id;

   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 9800 );

   //print_market( "", "" );
   auto unmatched = create_sell_order_with_ext( db, trx, buyer_account, core_asset.amount(1500), test_asset.amount(300), false );
   //print_market( "", "" );
   BOOST_CHECK( !db.find( first_id ) );
   BOOST_CHECK( !db.find( second_id ) );
   if( unmatched ) wdump((*unmatched));
   BOOST_CHECK( unmatched );

   //APM
   //sell_asset nathan 100 TEST 400 BTS 100000 false true    <-- seller SELL 100 TEST @ 4 (bts)
   //sell_asset nathan 100 TEST 300 BTS 100000 false true    <-- seller SELL 100 TEST @ 3
   //sell_asset nathan 1500 BTS 300 TEST 100000 false true    <-- buyer BUY 300 TEST @ 5
   //expected result: 100 TEST filled @3, 100 TEST filled @4, remainder: 100 TEST bid @5
   // seller is selling TEST buying CORE
   // buyer is buying TEST selling CORE
   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 9800 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 8800 );
   BOOST_CHECK_EQUAL( test_asset.dynamic_asset_data_id(db).accumulated_fees.value , 2 );
   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 700 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 198 );
 }
 catch ( const fc::exception& e )
 {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
 }
}

BOOST_AUTO_TEST_CASE( create_buy_uia_multiple_match_new )
{ try {
   INVOKE( issue_uia );
   const asset_object&   test_asset     = get_asset( "TEST" );
   const asset_object&   core_asset     = get_asset( GRAPHENE_SYMBOL );
   const account_object& nathan_account = get_account( "nathan" );
   const account_object& buyer_account  = create_account( "buyer" );
   const account_object& seller_account = create_account( "seller" );

   transfer( committee_account(db), buyer_account, core_asset.amount( 10000 ) );
   transfer( nathan_account, seller_account, test_asset.amount(10000) );

   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 10000 );

   limit_order_id_type first_id  = create_sell_order( seller_account, test_asset.amount(100), core_asset.amount(100) )->id;
   limit_order_id_type second_id = create_sell_order( seller_account, test_asset.amount(100), core_asset.amount(200) )->id;
   limit_order_id_type third_id  = create_sell_order( seller_account, test_asset.amount(100), core_asset.amount(300) )->id;

   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 9700 );

   //print_market( "", "" );
   auto unmatched = create_sell_order_with_ext( db, trx, buyer_account, core_asset.amount(300), test_asset.amount(150), false );
   //print_market( "", "" );
   BOOST_CHECK( !db.find( first_id ) );
   BOOST_CHECK( db.find( second_id ) );
   BOOST_CHECK( db.find( third_id ) );
   if( unmatched ) wdump((*unmatched));
   BOOST_CHECK( !unmatched );

   //APM
   //sell_asset nathan 100 TEST 100 BTS 100000 false true    <-- seller SELL 100 TEST @ 1 (bts)
   //sell_asset nathan 100 TEST 200 BTS 100000 false true    <-- seller SELL 100 TEST @ 2 (bts)
   //sell_asset nathan 100 TEST 300 BTS 100000 false true    <-- seller SELL 100 TEST @ 3 (bts)
   //sell_asset nathan 300 BTS 150 TEST 100000 false true    <-- buyer BUY 150 TEST @ 2 (bts)
   //expected result: 100 TEST filled @1, 50 TEST filled @2
   // seller is selling TEST buying CORE
   // buyer is buying TEST selling CORE
   BOOST_CHECK_EQUAL( get_balance( seller_account, test_asset ), 9700 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, core_asset ), 9800 );
   BOOST_CHECK_EQUAL( test_asset.dynamic_asset_data_id(db).accumulated_fees.value , 1 );
   BOOST_CHECK_EQUAL( get_balance( seller_account, core_asset ), 200 );
   BOOST_CHECK_EQUAL( get_balance( buyer_account, test_asset ), 149 );
 }
 catch ( const fc::exception& e )
 {
    elog( "${e}", ("e", e.to_detail_string() ) );
    throw;
 }
}


BOOST_AUTO_TEST_CASE( whalehole_test_1 )
{ try {
      ACTORS((buyer)(seller)(borrower)(borrower2)(feedproducer));

      int64_t init_balance(1000000);
      transfer(committee_account, buyer_id, asset(init_balance));

      transfer(committee_account, seller_id, asset(init_balance));

      asset_id_type core_id = db.get_index<asset_object>().get_next_id();
      asset_create_operation creator;
      creator.issuer = account_id_type();  //buyer.get_id();
      creator.fee = asset();
      creator.symbol = "CORE";
      creator.common_options.max_supply = 100000000;
      creator.precision = 8;
      creator.common_options.market_fee_percent = GRAPHENE_MAX_MARKET_FEE_PERCENT/100; /*1%*/
      //creator.common_options.issuer_permissions = charge_market_fee|white_list|override_authority|transfer_restricted|disable_confidential;
      //creator.common_options.flags = charge_market_fee|white_list|override_authority|disable_confidential;
      asset_options opts;
      opts.flags &= ~(white_list | disable_force_settle | global_settle);
      opts.issuer_permissions = opts.flags;
	  idump((opts.flags));
	  idump((opts.issuer_permissions));

      creator.common_options.core_exchange_rate = price({asset(2),asset(1,asset_id_type(1))});
      //creator.common_options.whitelist_authorities.insert(buyer);
	  //creator.common_options.whitelist_authorities.insert(seller);
	  //creator.common_options.whitelist_authorities = creator.common_options.blacklist_authorities = {account_id_type()};
      trx.operations.push_back(std::move(creator));
      PUSH_TX( db, trx, ~0 );
	  trx.clear();

	  const asset_object& core = core_id(db);

      asset_id_type whalehole_id = db.get_index<asset_object>().get_next_id();
      //asset_create_operation creator;
	  creator.issuer = account_id_type();  //buyer.get_id();
      creator.fee = asset();
      creator.symbol = "WHALEHOLE";
      creator.common_options.max_supply = 100000000;
      creator.precision = 0;
      creator.common_options.market_fee_percent = GRAPHENE_MAX_MARKET_FEE_PERCENT/100; /*1%*/
      //creator1.common_options.issuer_permissions = charge_market_fee|white_list|override_authority|transfer_restricted|disable_confidential;
      //creator1.common_options.flags = charge_market_fee|white_list|override_authority|disable_confidential;
      creator.common_options.core_exchange_rate = price({asset(2),asset(1,asset_id_type(1))});
      //creator1.common_options.whitelist_authorities.insert(buyer);
	  //creator1.common_options.whitelist_authorities.insert(seller);
	  //creator1.common_options.whitelist_authorities = creator1.common_options.blacklist_authorities = {account_id_type()};
	  trx.operations.push_back(std::move(creator));
      PUSH_TX( db, trx, ~0 );
	  trx.clear();
	  
	  const asset_object& whalehole = whalehole_id(db);

      asset_id_type mole_id = db.get_index<asset_object>().get_next_id();
      //asset_create_operation creator;
	  creator.issuer = account_id_type();  //buyer.get_id();
      creator.fee = asset();
      creator.symbol = "MOLE";
      creator.common_options.max_supply = 100000000;
      creator.precision = 4;
      creator.common_options.market_fee_percent = GRAPHENE_MAX_MARKET_FEE_PERCENT/100; /*1%*/
      //creator1.common_options.issuer_permissions = charge_market_fee|white_list|override_authority|transfer_restricted|disable_confidential;
      //creator1.common_options.flags = charge_market_fee|white_list|override_authority|disable_confidential;
      creator.common_options.core_exchange_rate = price({asset(2),asset(1,asset_id_type(1))});
      //creator1.common_options.whitelist_authorities.insert(buyer);
	  //creator1.common_options.whitelist_authorities.insert(seller);
	  //creator1.common_options.whitelist_authorities = creator1.common_options.blacklist_authorities = {account_id_type()};
	  trx.operations.push_back(std::move(creator));
      PUSH_TX( db, trx, ~0 );
	  trx.clear();

	  const asset_object& mole = mole_id(db);

      //const auto& bitusd = create_bitasset("USDBIT", feedproducer_id);
      //const auto& test  = asset_id_type()(db);



      asset usd_pays, usd_receives, core_pays, core_receives;
	  price match_price;
	  //price ask_core = price(asset(80,whalehole_id), asset(480,core_id));
	  //price bid_usd = price(asset(21,core_id), asset(2,whalehole_id));	
	
	// bid-taker
	limit_order_object ask_core = limit_order_object();
      ask_core.seller = seller.get_id();
	  ask_core.sell_price = price(asset(80,whalehole_id), asset(480,core_id));
	  ask_core.for_sale = 80;	
	limit_order_object bid_usd = limit_order_object();
	  bid_usd.seller = buyer.get_id();
	  bid_usd.sell_price = price(asset(21,core_id), asset(2,whalehole_id));
	  bid_usd.for_sale = 21;
	match_price = ask_core.sell_price;

	// ask-taker
	/*limit_order_object bid_usd = limit_order_object();
	  bid_usd.seller = seller.get_id();
	  bid_usd.sell_price = price(asset(80,whalehole_id), asset(480,core_id));
	  bid_usd.for_sale = 80;
	limit_order_object ask_core = limit_order_object();
	  ask_core.seller = buyer.get_id();
	  ask_core.sell_price = price(asset(21,core_id), asset(2,whalehole_id));
	  ask_core.for_sale = 21;
	match_price = bid_usd.sell_price;*/


     auto usd_for_sale = bid_usd.amount_for_sale();
     auto core_for_sale = ask_core.amount_for_sale();

	double real_match_price = match_price.to_real();
	double real_order_price = bid_usd.sell_price.to_real();
	double real_book_price = ask_core.sell_price.to_real();
	int64_t usd_max_counter_size = round(bid_usd.amount_to_receive().amount.value / real_book_price);
	if (usd_max_counter_size < usd_for_sale.amount) usd_for_sale.amount = usd_max_counter_size;
	int64_t core_max_counter_size = round(ask_core.amount_to_receive().amount.value * real_book_price);
	if (core_max_counter_size < core_for_sale.amount) core_for_sale.amount = core_max_counter_size;
	
	idump((usd_max_counter_size));
	idump((usd_for_sale));

    idump((core_max_counter_size));
	idump((core_for_sale));

	idump((ask_core));
	idump((bid_usd));
	idump((match_price));

   if( usd_for_sale <= core_for_sale * match_price )
   {
      core_receives = usd_for_sale;
      usd_receives  = usd_for_sale * match_price;
   }
   else
   {
      //This line once read: assert( core_for_sale < usd_for_sale * match_price );
      //This assert is not always true -- see trade_amount_equals_zero in operation_tests.cpp
      //Although usd_for_sale is greater than core_for_sale * match_price, core_for_sale == usd_for_sale * match_price
      //Removing the assert seems to be safe -- apparently no asset is created or destroyed.
      usd_receives = core_for_sale;
      core_receives = core_for_sale * match_price;
   }
   core_pays = usd_receives;
   usd_pays  = core_receives;
	  
   idump((core_receives));
   idump((usd_receives));
   idump((core_pays));
   idump((usd_pays));
  
   bool result = false;

   //step 1 is in match(), step 2 is in fill_order()

   double real_taker_price = bid_usd.sell_price.to_real();
   int64_t real_taker_over = round(usd_receives.amount.value * real_taker_price) - usd_pays.amount.value;
   bid_usd.for_sale -= usd_pays.amount;
   ask_core.for_sale -= core_pays.amount;
   if (real_taker_over > 0) bid_usd.for_sale -= real_taker_over;
   
   idump((bid_usd));
   idump((ask_core));

   //try { result |= db.fill_order( bid_usd, usd_pays, usd_receives, false ); } catch(...) {}
   //try { result |= db.fill_order( ask_core, core_pays, core_receives, true ) << 1; } catch(...) {}

	  
	  issue_uia(buyer, core.amount(10000000));
	  issue_uia(buyer, whalehole.amount(10000));
	  issue_uia(buyer, mole.amount(10000000));
	  issue_uia(seller, core.amount(10000000));
	  issue_uia(seller, whalehole.amount(10000));
	  issue_uia(seller, mole.amount(10000000));


      auto order = create_sell_order( buyer, asset(925,core_id), asset(1,mole_id));
      order = create_sell_order( seller, asset(950,mole_id), asset(1,core_id));
	  order = create_sell_order( seller, asset(9393000,mole_id), asset(101,core_id));

      //auto order = create_sell_order( seller, asset(480,core_id), asset(80,whalehole_id));
      //order = create_sell_order( buyer, asset(4,whalehole_id), asset(20,core_id));
	  //order = create_sell_order( buyer, asset(2,whalehole_id), asset(17,core_id));
      order = create_sell_order( seller, asset(80,whalehole_id), asset(480,core_id));
      order = create_sell_order( buyer, asset(20,core_id), asset(4,whalehole_id));
	  order = create_sell_order( buyer, asset(17,core_id), asset(2,whalehole_id));
	  order = create_sell_order( buyer, asset(21,core_id), asset(2,whalehole_id));
      
	  order = create_sell_order( seller, asset(2,whalehole_id), asset(20,core_id));
	  order = create_sell_order( seller, asset(5,whalehole_id), asset(20,core_id));


	  ilog("done!");

   } catch( const fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}



// TODO:  Write linear VBO tests

BOOST_AUTO_TEST_SUITE_END()

