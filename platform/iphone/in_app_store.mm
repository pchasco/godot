/*************************************************************************/
/*  in_app_store.mm                                                      */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#ifdef STOREKIT_ENABLED
#include "in_app_store.h"

extern "C" {
#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>
};

@interface SKProduct (LocalizedPrice)
@property(nonatomic, readonly) NSString *localizedPrice;
@end

//----------------------------------//
// SKProduct extension
//----------------------------------//
@implementation SKProduct (LocalizedPrice)
- (NSString *)localizedPrice {
	NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
	[numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
	[numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
	[numberFormatter setLocale:self.priceLocale];
	NSString *formattedString = [numberFormatter stringFromNumber:self.price];
	[numberFormatter release];
	return formattedString;
}
@end

@interface ProductsDelegate : NSObject <SKProductsRequestDelegate>
// ProductsDelegate doesn't define any additional members
@end

@implementation ProductsDelegate

- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response {
	// Build parallel arrays of each property of each item in response
	StringArray titles;
	StringArray descriptions;
	RealArray prices;
	StringArray ids;
	StringArray localized_prices;

	NSArray *products = response.products;

	for (int i = 0; i < [products count]; i++) {
		SKProduct *product = [products objectAtIndex:i];

		const char *str = [product.localizedTitle UTF8String];
		titles.push_back(String::utf8(str != NULL ? str : ""));
		str = [product.localizedDescription UTF8String];
		descriptions.push_back(String::utf8(str != NULL ? str : ""));
		prices.push_back([product.price doubleValue]);
		ids.push_back(String::utf8([product.productIdentifier UTF8String]));
		localized_prices.push_back(String::utf8([product.localizedPrice UTF8String]));
	};

	StringArray invalid_ids;
	for (NSString *ipid in response.invalidProductIdentifiers) {
		invalid_ids.push_back(String::utf8([ipid UTF8String]));
	};

	Dictionary ret;
	ret["type"] = "product_info";
	ret["result"] = "ok";
	ret["titles"] = titles;
	ret["descriptions"] = descriptions;
	ret["prices"] = prices;
	ret["ids"] = ids;
	ret["localized_prices"] = localized_prices;
	ret["invalid_ids"] = invalid_ids;

	InAppStore::get_singleton()->_post_event(ret);

	[request release];
};

@end


@interface TransObserver : NSObject <SKPaymentTransactionObserver>
- (void) handlePaymentTransactionStatePurchasing:(SKPaymentTransaction *)transaction;
- (void) handlePaymentTransactionStatePurchased:(SKPaymentTransaction *)transaction;
- (void) handlePaymentTransactionStateDeferred:(SKPaymentTransaction *)transaction;
- (void) handlePaymentTransactionStateFailed:(SKPaymentTransaction *)transaction;
- (void) handlePaymentTransactionStateRestored:(SKPaymentTransaction *)transaction;
@end

NSMutableDictionary *open_transactions = [[NSMutableDictionary alloc] init];

@implementation TransObserver

- (void) handlePaymentTransactionStatePurchasing:(SKPaymentTransaction *)transaction {
	NSLog(@"InAppStore: Received: handlePaymentTransactionStatePurchasing");
}

- (void) handlePaymentTransactionStatePurchased:(SKPaymentTransaction *)transaction {
	NSLog(@"InAppStore: Received: handlePaymentTransactionStatePurchased");

	String productIdentifier = String::utf8([transaction.payment.productIdentifier UTF8String]);
	String transactionId = String::utf8([transaction.transactionIdentifier UTF8String]);
	InAppStore::get_singleton()->_record_purchase(productIdentifier);

	Dictionary ret;
	ret["type"] = "purchase";
	ret["result"] = "ok";
	ret["product_id"] = productIdentifier;
	ret["transaction_id"] = transactionId;

	InAppStore::get_singleton()->_post_event(ret);

	[open_transactions setObject:transaction forKey:transaction.transactionIdentifier];
}

- (void) handlePaymentTransactionStateDeferred:(SKPaymentTransaction *)transaction {
	NSLog(@"InAppStore: Received: handlePaymentTransactionStateDeferred");
}

- (void) handlePaymentTransactionStateFailed:(SKPaymentTransaction *)transaction {
	NSLog(@"InAppStore: Received: handlePaymentTransactionStateFailed");

	String pid = String::utf8([transaction.payment.productIdentifier UTF8String]);
	String errorMessage = String::utf8([transaction.error.localizedDescription UTF8String]);
	Dictionary event;
	event["type"] = "purchase";
	event["result"] = "error";
	event["product_id"] = pid;
	event["message"] = errorMessage;
	InAppStore::get_singleton()->_post_event(event);
}

- (void) handlePaymentTransactionStateRestored:(SKPaymentTransaction *)transaction {
	NSLog(@"InAppStore: Received: handlePaymentTransactionStateRestored");

	String transactionId = String::utf8([transaction.transactionIdentifier UTF8String]);
	String productIdentifier = String::utf8([transaction.originalTransaction.payment.productIdentifier UTF8String]);
	InAppStore::get_singleton()->_record_purchase(productIdentifier);	

	Dictionary event;
	event["type"] = "restore";
	event["result"] = "ok";
	event["product_id"] = productIdentifier;
	event["transaction_id"] = transactionId;
	InAppStore::get_singleton()->_post_event(event);
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions {
	for (SKPaymentTransaction *transaction in transactions) {
		switch (transaction.transactionState) {
			case SKPaymentTransactionStatePurchasing:
				[self handlePaymentTransactionStatePurchasing];
				break;
			case SKPaymentTransactionStatePurchased:
				[self handlePaymentTransactionStatePurchased];
				break;
			case SKPaymentTransactionStateDeferred:
				[self handlePaymentTransactionStateDeferred];
				break;
			case SKPaymentTransactionStateFailed: 
				[self handlePaymentTransactionStateFailed];
				break;
			case SKPaymentTransactionStateRestored: 
				[self handlePaymentTransactionStateRestored];
				break;
			default: {
				NSLog(@"InAppStore: Received unknown SKPaymentTransaction.transactionState %i!\n", (int)transaction.transactionState);
			}; break;
		};
	};
};

@end

InAppStore *InAppStore::instance = NULL;

void InAppStore::_bind_methods() {
	ObjectTypeDB::bind_method(_MD("request_product_info"), &InAppStore::request_product_info);
	ObjectTypeDB::bind_method(_MD("purchase"), &InAppStore::purchase);
	ObjectTypeDB::bind_method(_MD("restore_purchases"), &InAppStore::restore_purchases);
	ObjectTypeDB::bind_method(_MD("finish_transaction"), &InAppStore::finish_transaction);
	ObjectTypeDB::bind_method(_MD("is_product_purchased"), &InAppStore::is_product_purchased);

	ObjectTypeDB::bind_method(_MD("get_pending_event_count"), &InAppStore::get_pending_event_count);
	ObjectTypeDB::bind_method(_MD("pop_pending_event"), &InAppStore::pop_pending_event);
};

Error InAppStore::request_product_info(Variant p_params) {
	Dictionary params = p_params;
	ERR_FAIL_COND_V(!params.has("product_ids"), ERR_INVALID_PARAMETER);

	StringArray pids = params["product_ids"];

	NSMutableArray *array = [[[NSMutableArray alloc] initWithCapacity:pids.size()] autorelease];
	for (int i = 0; i < pids.size(); i++) {
		NSString *pid = [[[NSString alloc] initWithUTF8String:pids[i].utf8().get_data()] autorelease];
		[array addObject:pid];
	};

	NSSet *products = [[[NSSet alloc] initWithArray:array] autorelease];
	SKProductsRequest *request = [[SKProductsRequest alloc] initWithProductIdentifiers:products];

	ProductsDelegate *delegate = [[ProductsDelegate alloc] init];

	request.delegate = delegate;
	[request start];

	return OK;
};

Error InAppStore::purchase(Variant p_params) {
	ERR_FAIL_COND_V(![SKPaymentQueue canMakePayments], ERR_UNAVAILABLE);
	if (![SKPaymentQueue canMakePayments])
		return ERR_UNAVAILABLE;

	Dictionary params = p_params;
	ERR_FAIL_COND_V(!params.has("product_id"), ERR_INVALID_PARAMETER);

	NSString *pid = [[[NSString alloc] initWithUTF8String:String(params["product_id"]).utf8().get_data()] autorelease];
	SKPayment *payment = [SKPayment paymentWithProductIdentifier:pid];
	SKPaymentQueue *defq = [SKPaymentQueue defaultQueue];
	[defq addPayment:payment];

	return OK;
};

Error InAppStore::restore_purchases() {
	[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

int InAppStore::get_pending_event_count() {
	return pending_events.size();
};

Variant InAppStore::pop_pending_event() {
	if (pending_events.size() == 0) {
		Variant empty;
		return empty;
	}

	Variant front = pending_events.front()->get();
	pending_events.pop_front();
	return front;
};

void InAppStore::_post_event(Variant p_event) {
	pending_events.push_back(p_event);
};

void InAppStore::_record_purchase(String product_id) {
	String skey = "purchased/" + product_id;
	NSString *key = [[[NSString alloc] initWithUTF8String:skey.utf8().get_data()] autorelease];
	[[NSUserDefaults standardUserDefaults] setBool:YES forKey:key];
	[[NSUserDefaults standardUserDefaults] synchronize];
};

bool InAppStore::is_product_purchased(String product_id) {
	String skey = "purchased/" + product_id;
	NSString *key = [[[NSString alloc] initWithUTF8String:skey.utf8().get_data()] autorelease];
	return (bool)[[NSUserDefaults standardUserDefaults] boolForKey:key];
}

InAppStore *InAppStore::get_singleton() {
	return instance;
};

InAppStore::InAppStore() {
	ERR_FAIL_COND(instance != NULL);
	instance = this;

	TransObserver *observer = [[TransObserver alloc] init];
	[[SKPaymentQueue defaultQueue] addTransactionObserver:observer];
};

void InAppStore::finish_transaction(String transaction_id) {
	NSString *tid = [NSString stringWithCString:transaction_id.utf8().get_data() encoding:NSUTF8StringEncoding];

	NSObject *transaction = [open_transactions objectForKey:tid];
	if (transaction) {
		[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
		[open_transactions removeObjectForKey:tid];
	}
};

InAppStore::~InAppStore(){

};

#endif
