/***************************************************************
 *
 * (C) 2011-13 Nicola Bonelli <nicola.bonelli@cnit.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 ****************************************************************/

#ifndef _FUNCTIONAL_INLINE_H_
#define _FUNCTIONAL_INLINE_H_

#include <pf_q-engine.h>

#include "predicate.h"
#include "combinator.h"
#include "conditional.h"
#include "filter.h"
#include "forward.h"

/* INLINE_FUN macro */

#ifdef PFQ_USE_INLINE_FUN
#define INLINE_FUN(fun) 	(void *)INLINE_ ## fun
#else
#define INLINE_FUN(fun) 	&fun
#endif

#ifdef PFQ_USE_INLINE_FUN

/* high order functions */

#define INLINE_unit 			1
#define INLINE_conditional 		2
#define INLINE_mark 			3
#define INLINE_when 			4
#define INLINE_unless 			5

/* filter functions */

#define INLINE_filter_ip        	6
#define INLINE_filter_ip6       	7
#define INLINE_filter_udp       	8
#define INLINE_filter_icmp      	9
#define INLINE_filter_tcp       	10
#define INLINE_filter_udp6       	11
#define INLINE_filter_tcp6       	12
#define INLINE_filter_icmp6      	13
#define INLINE_filter_flow      	14
#define INLINE_filter_vlan      	15

/* forward functions */

#define INLINE_forward_drop            	16
#define INLINE_forward_broadcast       	17
#define INLINE_forward_kernel 	       	18
#define INLINE_forward_class 		19

/* combinator functions */

#define INLINE_or 			100
#define INLINE_and 			101
#define INLINE_xor 			102

/* predicate functions */

#define INLINE_less 			200
#define INLINE_less_eq 			201
#define INLINE_equal			202
#define INLINE_not_equal	       	203
#define INLINE_greater 			204
#define INLINE_greater_eq  		205
#define INLINE_any_bit 			206
#define INLINE_all_bit			207



#define CASE_INLINE(f, call, skb) \
	case INLINE_ ## f: ret = f(call,skb); break;

#define EVAL_FUNCTION(call, skb) ({\
	typeof(call->fun(skb)) ret; \
	switch((ptrdiff_t)call->fun) \
	{ 	\
		CASE_INLINE(unit, call, skb);\
		CASE_INLINE(mark, call, skb);\
		CASE_INLINE(conditional, call, skb);\
		CASE_INLINE(when, call, skb);\
		CASE_INLINE(unless, call, skb);\
		\
		CASE_INLINE(filter_ip, call, skb);\
		CASE_INLINE(filter_udp, call, skb);\
		CASE_INLINE(filter_tcp, call, skb);\
		CASE_INLINE(filter_icmp, call, skb);\
		CASE_INLINE(filter_ip6, call, skb);\
		CASE_INLINE(filter_udp6, call, skb);\
		CASE_INLINE(filter_tcp6, call, skb);\
		CASE_INLINE(filter_icmp6, call, skb);\
		CASE_INLINE(filter_flow, call, skb);\
		CASE_INLINE(filter_vlan, call, skb);\
		\
		CASE_INLINE(forward_drop, call, skb);\
		CASE_INLINE(forward_broadcast, call, skb);\
		CASE_INLINE(forward_kernel, call, skb);\
		CASE_INLINE(forward_class, call, skb);\
		default: ret = call->fun(skb); \
	} \
	ret; })


#define EVAL_PREDICATE(call, skb) ({ \
	typeof(call->fun(skb)) ret; \
	switch((ptrdiff_t)call->fun) \
	{ 	\
		CASE_INLINE(less,  call, skb);\
		CASE_INLINE(less_eq, call, skb);\
		CASE_INLINE(equal, call, skb);\
		CASE_INLINE(not_equal, call, skb);\
		CASE_INLINE(greater, call, skb);\
		CASE_INLINE(greater_eq, call, skb);\
		CASE_INLINE(any_bit, call, skb);\
		CASE_INLINE(all_bit, call, skb);\
		default: ret = call->fun(skb); \
	} \
	ret; })


#define RETURN_EVAL_COMBINATOR(call, skb) ({ \
	typeof(call->fun(skb)) ret; \
	switch((ptrdiff_t)call->fun) \
	{ 	\
		CASE_INLINE(or,  call, skb);\
		CASE_INLINE(and, call, skb);\
		CASE_INLINE(xor, call, skb);\
	} \
	ret; })

#endif

#endif /* _FUNCTIONAL_INLINE_H_ */
