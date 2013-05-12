#ifndef __PLAT_MESON_RESOURCE_H_
#define __PLAT_MESON_RESOURCE_H_
#include <linux/types.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define IgnorWarning()
#else
#define IgnorWarning() _Pragma("GCC diagnostic ignored \"-Wdeclaration-after-statement\"")
///#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
#endif
struct cpu_module_object{
	struct list_head list;
	const char * name;
	struct platform_device device;
#if 0	
	u32		num_resources;
	struct resource	* resource;
	void * platform_data;///default platform data 
#endif	
};

struct cpu_module_class{
	struct list_head list;
	struct list_head child;
	const char * name;
	const char * driver; ///platform_driver name
};

#define module_class(class,drv)	.name=#class,.driver=#drv
#define mesonplat_cpu_resource(fn,module_class...)					\
	IgnorWarning()													\
	static int __init fn(void)     									\
	{   int ret=0;													\
		static struct cpu_module_class   class={module_class}		
#define mesonplat_cpu_resource_initcall_end(statge,class,drv)   return ret;}

#define mesonplat_cpu_resource_initcall_begin(stage,class,drv)				\
		__do_mesonplat_cpu_resource_initcall(stage,class##_##stage##_fn,	\
			module_class(class,drv))
	
#define __do_mesonplat_cpu_resource_initcall(stage,fn,class)		\
	static int __init fn(void) ;    								\
	stage##_initcall(fn);											\
	mesonplat_cpu_resource(fn,class)    								
	

#define module_object(object,res,platformdata)							\
		static struct resource resource_##object[]=res;            		\
		static struct cpu_module_object object_##object={               \
			.name=#object,												\
			.device.id=-1, 		                                        \
			.device.num_resources=ARRAY_SIZE(resource_##object), 		\
			.device.resource=&resource_##object[0], 		            \
			.device.dev.platform_data=platformdata						\
		};mesonplat_cpu_resource_add(&class,&object_##object)

#define mesonplat_cpu_resource_early_initcall_begin(earlyparam,cls,drv) 	\
	static int __init class##_##earlyparam##_fn(char *buf) ;    		\
	early_param(#earlyparam,class##_##earlyparam##_fn);					\
	static int __init class##_##earlyparam##_fn(char *buf)     		\
	{	int ret=0;																\
		static struct cpu_module_class   class={module_class(cls,drv)}		


#define mesonplat_cpu_resource_early_initcall_end(earlyparam,class,drv)	\
	mesonplat_cpu_resource_initcall_end(early,class,drv)

#define mesonplat_cpu_resource_pure_initcall_begin(class,drv) mesonplat_cpu_resource_initcall_begin(pure,class,drv)
#define mesonplat_cpu_resource_pure_initcall_end(class,drv) mesonplat_cpu_resource_initcall_end(pure,class,drv)

#define mesonplat_cpu_resource_modify_initcall_begin(class,drv) mesonplat_cpu_resource_initcall_begin(core,class,drv)
#define mesonplat_cpu_resource_modify_initcall_end(class,drv) mesonplat_cpu_resource_initcall_end(core,class,drv)

#define mesonplat_cpu_resource_modify_initcall(fn) core_initcall(fn)

int32_t mesonplat_cpu_resource_get_num(const char * name);
struct cpu_module_object * mesonplat_cpu_resource_get(const char * cls,const char * obj);
int32_t mesonplat_cpu_resource_add(struct cpu_module_class * cls,struct cpu_module_object* obj);
int32_t mesonplat_resource_modify(const char * cls_name,const char * obj_name,	int id,
	struct resource	* resource,
	u32		num_resources,
	void * platform_data);
int32_t mesonplat_register_device(const char * cls,const char * obj,void * platform_data);
int32_t mesonplat_register_device_early(const char * cls,const char * obj,void * platform_data);


#endif
