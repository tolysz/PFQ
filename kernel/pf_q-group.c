/***************************************************************
 *                                                
 * (C) 2011-12 Nicola Bonelli <nicola.bonelli@cnit.it>   
 *             Andrea Di Pietro <andrea.dipietro@for.unipi.it>
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/semaphore.h>

#include <pf_q-group.h>
#include <pf_q-devmap.h>
#include <pf_q-bits.h>

DEFINE_SEMAPHORE(group_sem);


struct pfq_group pfq_groups[Q_MAX_GROUP];

/* precondition: gid must be valid */


static bool 
__pfq_is_joinable(int gid, int policy)
{
        struct pfq_group * that = &pfq_groups[gid];

	if (!that->pid)
		return true;

	if (that->pid != -1) { /* restricted group */
        	return that->pid == current->tgid;	
	}
	else {	/* shared group */
        	if (policy == Q_GROUP_RESTRICTED) {
			return __pfq_group_is_empty(gid);
		}
		else {
                 	return true;
		}
	}
}


static void 
__pfq_group_ctor(int gid)
{
        struct pfq_group * that = &pfq_groups[gid];
	int i;

        that->pid = -1;
	
	for(i = 0; i < Q_CLASS_MAX; i++)
	{
		atomic_long_set(&that->sock_mask[i], 0);
	}

	atomic_long_set(&that->steering, 0);

	sparse_set(&that->recv, 0);
	sparse_set(&that->lost, 0);
	sparse_set(&that->drop, 0);
}


static void 
__pfq_group_dtor(int gid)
{
        struct pfq_group * that = &pfq_groups[gid];

	/* remove this gid from demux matrix */
        pfq_devmap_update(map_reset, Q_ANY_DEVICE, Q_ANY_QUEUE, gid);
	
	that->pid = 0;
}



static int
__pfq_join_group(int gid, int id, unsigned long class_mask, int policy)
{
        unsigned long tmp = 0;
        unsigned int class;

        if (!pfq_groups[gid].pid) {
         	__pfq_group_ctor(gid);
	}
	
	if (!__pfq_is_joinable(gid, policy)) {
		printk(KERN_INFO "[PFQ] gid:%d is not joinable with policy %d\n", gid, policy);
		return -1;
	}

	bitwise_for_each(class_mask, class)
	{
		tmp = atomic_long_read(&pfq_groups[gid].sock_mask[class]);
		tmp |= 1L << id;
		atomic_long_set(&pfq_groups[gid].sock_mask[class], tmp);
	}
	
	pfq_groups[gid].pid = (policy == Q_GROUP_RESTRICTED ? current->tgid : -1);
        return 0;
}


static int
__pfq_leave_group(int gid, int id)
{
        unsigned long tmp;
	int i;

	if (!pfq_groups[gid].pid)
		return -1;
	
	for(i = 0; i < Q_CLASS_MAX; ++i)
	{
		tmp = atomic_long_read(&pfq_groups[gid].sock_mask[i]);
		tmp &= ~(1L << id);
		atomic_long_set(&pfq_groups[gid].sock_mask[i], tmp);
	}

	if (__pfq_group_is_empty(gid)) {
        	__pfq_group_dtor(gid);
	}
        
        return 0;
}

unsigned long
__pfq_get_all_groups_mask(int gid)
{
	unsigned long mask = 0;
	int i;
	for(i = 0; i < Q_CLASS_MAX; ++i)
	{
		mask |= atomic_long_read(&pfq_groups[gid].sock_mask[i]);
	}
	return mask;
}


int
pfq_join_group(int gid, int id, unsigned long class_mask, int policy)
{
	int ret;
	down(&group_sem);

	ret = __pfq_join_group(gid, id, class_mask, policy);

	up(&group_sem);
	return ret;
}


int 
pfq_join_free_group(int id, unsigned long class_mask, int policy)
{
        int n = 0;
        down(&group_sem);
        for(; n < Q_MAX_ID; n++)
        {            
                if(!pfq_groups[n].pid)
                {
			__pfq_join_group(n, id, class_mask, policy);
                        up(&group_sem);
                        return n;
                }
        }
        up(&group_sem);
        return -1;
}



int
pfq_leave_group(int gid, int id)
{
 	int ret;
        down(&group_sem);
        ret = __pfq_leave_group(gid,id);
	up(&group_sem);
	return ret;
}


void
pfq_leave_all_groups(int id)
{
        int n = 0;
        down(&group_sem);
        for(; n < Q_MAX_ID; n++)
        {            
		__pfq_leave_group(n, id);
        }
        up(&group_sem);
}


unsigned long
pfq_get_groups(int id)
{
        unsigned long ret = 0;
        int n = 0;
        down(&group_sem);
        for(; n < Q_MAX_ID; n++)
        {       
		unsigned long mask = __pfq_get_all_groups_mask(n);
                if(mask & (1L << id))
                {
                        ret |= (1UL << n);
                }
        }
        up(&group_sem);
        return ret;
}


