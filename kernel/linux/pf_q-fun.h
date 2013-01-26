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

#ifndef _PF_Q_FUN_H_
#define _PF_Q_FUN_H_ 

#include <linux/pf_q.h>

#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>


typedef struct
{
	unsigned int  hash:24;
	unsigned int  type:8;
	unsigned int  class;
} funret_t;



enum action 
{
    action_drop      = 0x01,
    action_clone     = 0x02,
    action_dispatch  = 0x04,
    action_steal     = 0x08,
    action_continue  = 0x10,
    action_to_kernel = 0x80,
};


typedef funret_t (*sk_function_t)(const struct sk_buff *, const void *);    


struct sk_function_descr
{
    const char *      name;
    sk_function_t     function;
};


extern int pfq_register_functions  (const char *module, struct sk_function_descr *fun);
extern int pfq_unregister_functions(const char *module, struct sk_function_descr *fun);


/* none: ignore the packet for the current group */

static inline
funret_t none(void)
{
    funret_t ret = { 0, action_drop, 0};
    return ret;
}


/* continuation: pass the skb to the next sk function */

static inline
funret_t pass(void)
{
    funret_t ret = { 0, action_continue, 0};
    return ret;
}


/* broadcast: for this group, broadcast the skb to sockets of the given classes */

static inline
funret_t broadcast(unsigned int cl)
{
    funret_t ret = { 0, action_clone, cl};
    return ret;
}


/* steering skb: for this group, dispatch the skb across sockets of the given classes (by means of hash) */

static inline
funret_t steering(unsigned int cl, unsigned int hash)
{
    funret_t ret = {hash ^ (hash >> 8), action_dispatch, cl};
    return ret;
}


/* stolen packet: the skb is stolen by the steering function. (i.e. forwarded) */

static inline
funret_t stolen(void)
{
    funret_t ret = { 0, action_steal, 0};
    return ret;
}


/* modifier */

/* to_kernel: set the skb to be passed to kernel */

static inline
funret_t to_kernel(funret_t ret)
{
    if (unlikely(ret.type & action_steal))
    {
        if (printk_ratelimit())
 	        pr_devel("[PFQ] to_kernel modifier applied to a stolen packet!\n");
       return ret;
    }
    ret.type |= action_to_kernel;
    return ret;
}


#endif /* _PF_Q_FUN_H_ */