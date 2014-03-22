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

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/list.h>
#include <linux/string.h>
#include <linux/semaphore.h>

#include <linux/pf_q-module.h>

#include <pf_q-group.h>
#include <pf_q-factory.h>


DEFINE_SEMAPHORE(funfactory_sem);


struct function_factory_elem
{
	struct list_head 	function_list;
	char 			name[Q_FUN_NAME_LEN];
	pfq_function_t 	function;
};

extern struct pfq_function_descr filter_functions[];
extern struct pfq_function_descr forward_functions[];
extern struct pfq_function_descr steering_functions[];


LIST_HEAD(function_factory);

/*
 * register the funcitons here!
 */

int
pfq_register_functions(const char *module, struct pfq_function_descr *fun)
{
	int i = 0;
	for(; fun[i].name != NULL; i++)
	{
		pfq_register_function(module, fun[i].name, fun[i].function);
	}
	return 0;
}

int
pfq_unregister_functions(const char *module, struct pfq_function_descr *fun)
{
	int i = 0;
	for(; fun[i].name != NULL; i++)
	{
		pfq_unregister_function(module, fun[i].name);
	}
	return 0;
}

/*
 * register the default functions!
 */

void
pfq_function_factory_init(void)
{
        pfq_register_functions(NULL, filter_functions);
        pfq_register_functions(NULL, forward_functions);
        pfq_register_functions(NULL, steering_functions);

	printk(KERN_INFO "[PFQ] function-factory initialized.\n");
}


void
pfq_function_factory_free(void)
{
	struct list_head *pos = NULL, *q;
	struct function_factory_elem *this;

	down(&funfactory_sem);
	list_for_each_safe(pos, q, &function_factory)
	{
    		this = list_entry(pos, struct function_factory_elem, function_list);
		list_del(pos);
		kfree(this);
	}
	up(&funfactory_sem);
	printk(KERN_INFO "[PFQ] function factory freed.\n");
}


pfq_function_t
__pfq_get_function(const char *name)
{
	struct list_head *pos = NULL;
	struct function_factory_elem *this;

        if (name == NULL)
                return NULL;

	list_for_each(pos, &function_factory)
	{
    		this = list_entry(pos, struct function_factory_elem, function_list);
        	if (!strcmp(this->name, name))
			return this->function;
	}
	return NULL;
}


pfq_function_t
pfq_get_function(const char *name)
{
	pfq_function_t ret;
	down(&funfactory_sem);
	ret = __pfq_get_function(name);
	up(&funfactory_sem);
	return ret;
}


int
__pfq_register_function(const char *name, pfq_function_t fun)
{
	struct function_factory_elem * elem;

	if (__pfq_get_function(name) != NULL) {
		pr_devel("[PFQ] function factory error: name %s already in use!\n", name);
		return -1;
	}

	elem = kmalloc(sizeof(struct function_factory_elem), GFP_KERNEL);
	if (elem == NULL) {
		printk(KERN_WARNING "[PFQ] function factory error: out of memory!\n");
		return -1;
	}

	INIT_LIST_HEAD(&elem->function_list);

	elem->function = fun;

	strncpy(elem->name, name, Q_FUN_NAME_LEN-1);
        elem->name[Q_FUN_NAME_LEN-1] = '\0';
	list_add(&elem->function_list, &function_factory);

	return 0;
}


int
pfq_register_function(const char *module, const char *name, pfq_function_t fun)
{
	int r;
	down(&funfactory_sem);
	r = __pfq_register_function(name, fun);
	up(&funfactory_sem);
	if (r == 0 && module)
		printk(KERN_INFO "[PFQ]%s '%s' @%p function registered.\n", module, name, fun);

	return r;
}


int
__pfq_unregister_function(const char *name)
{
	struct list_head *pos = NULL, *q;
	struct function_factory_elem *this;

	list_for_each_safe(pos, q, &function_factory)
	{
    		this = list_entry(pos, struct function_factory_elem, function_list);
		if (!strcmp(this->name, name))
		{
			list_del(pos);
	       		kfree(this);
			return 0;
		}
	}
	pr_devel("[PFQ] function factory error: %s no such function\n", name);
	return -1;
}


int
pfq_unregister_function(const char *module, const char *name)
{
	pfq_function_t fun;

	down(&funfactory_sem);

	fun = __pfq_get_function(name);
	if (fun == NULL) {
        	return -1;
	}

	__pfq_dismiss_function(fun);
        __pfq_unregister_function(name);

	printk(KERN_INFO "[PFQ]%s '%s' function unregistered.\n", module, name);
	up(&funfactory_sem);

	return 0;
}

