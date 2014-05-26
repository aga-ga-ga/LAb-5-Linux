#define SYSFS


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>

#define WRITE_SIZE 100

#define ARG1 "arg1"
#define ARG2 "arg2"
#define OPERATION "operation"
#define RESULT "result"

#define PARENT_DIR "calc"

static char arg1_input[WRITE_SIZE];
static char arg2_input[WRITE_SIZE];
static char operation_input[WRITE_SIZE];

long calculate(void) {
	long a1 = 0;
	long a2 = 0;
	long res = 0;

	if (arg1_input[strlen(arg1_input) - 2] == '\n') {
		arg1_input[strlen(arg1_input) - 2] = (char)0;
	}
	//string -> long
	kstrtol(arg1_input, 10, &a1);
	kstrtol(arg2_input, 10, &a2);

	if (operation_input[0] == '+') {
		res = a1 + a2;
	} else if (operation_input[0] == '-') {
		res = a1 - a2;
	} else if (operation_input[0] == '.') {
		res = a1 * a2;
	} else if (operation_input[0] == '/') {
		res = a1 / a2;
	}
	return res;
}


#ifdef SYSFS

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>

static struct attribute arg1 = {
	.name = ARG1,
	.mode = 0666,
};

static struct attribute arg2 = {
	.name = ARG2,
	.mode = 0666,
};

static struct attribute operation = {
	.name = OPERATION,
	.mode = 0666,
};

static struct attribute result = {
	.name = RESULT,
	.mode = 0666,
};

static struct attribute * calc_attributes[] = {
	&arg1,
	&arg2,
	&operation,
	&result,
	NULL
};


static ssize_t default_show(struct kobject *kobj, struct attribute *attr,
		char *buf)
{
	if (!strcmp(attr->name, RESULT)) {
		long res = calculate();

		return sprintf(buf, "%ld\n", res);
	} else {
		return 0;
	}
}

static ssize_t default_store(struct kobject *kobj, struct attribute *attr,
		const char *buf, size_t len)
{
	if(len > WRITE_SIZE) {
		len = WRITE_SIZE;
	}

	if (!strcmp(attr->name, ARG1)) {
		memcpy(arg1_input, buf, len);
	} else if (!strcmp(attr->name, ARG2)) {
		memcpy(arg2_input, buf, len);
	} else if (!strcmp(attr->name, OPERATION)) {
		memcpy(operation_input, buf, len);
	}
	return len;
}

static struct sysfs_ops calc_ops = {
	.show = default_show,
	.store = default_store,
};

static struct kobj_type calc_type = {
	.sysfs_ops = &calc_ops,
	.default_attrs = calc_attributes,
};
//Во внешнем представлении в каталоге /sys каждому объекту struct kobject соответствует каталог, что видно и из самого определения структуры
//Но это вовсе не означает, что каждый инициализированный объект struct kobject автоматически экспортируется в файловую систему /sys. 
//Для того, чтобы сделать объект видимым в /sys, необходимо вызвать: kobject_add
struct kobject *calc_obj;
static int __init sysfsexample_module_init(void)
{
	int err = -1;
	//GFP_KERNE  Это обычный запрос, который может блокировать поток. Данный флаг используется при выделении памяти в контексте потока, который может заснуть.
	calc_obj = kzalloc(sizeof(*calc_obj), GFP_KERNEL);
	if (calc_obj) {
		kobject_init(calc_obj, &calc_type);
		if (kobject_add(calc_obj, NULL, "%s", PARENT_DIR)) {
			 err = -1;
			 printk("Sysfs creation failed\n");
			 //вызов kobject_put () будет уменьшать 
			 //счетчик ссылок и, возможно, освободить объект calc_obj
			 kobject_put(calc_obj);
			 calc_obj = NULL;
		}
		err = 0;
	}
	return err;
}

static void __exit sysfsexample_module_exit(void)
{
	if (calc_obj) {
		kobject_put(calc_obj);
		kfree(calc_obj);
	}
}

module_init(sysfsexample_module_init);
module_exit(sysfsexample_module_exit);
MODULE_LICENSE("GPL");


#else

#include <linux/proc_fs.h>

struct proc_dir_entry *calc_dir;
struct proc_dir_entry *arg1;
struct proc_dir_entry *arg2;
struct proc_dir_entry *operation;
struct proc_dir_entry *result;


/*
 * arg1 write handler
 */
int write_arg1(struct file *file, const char *buf, unsigned long count, void *data)
{
	if(count > WRITE_SIZE) {
    		count = WRITE_SIZE;
	}
	memcpy(arg1_input, buf, count);
	return count;
}

/*
 * arg2 write handler
 */
int write_arg2(struct file *file, const char *buf, unsigned long count, void *data)
{
	if(count > WRITE_SIZE) {
    		count = WRITE_SIZE;
	}
	memcpy(arg2_input, buf, count);
	return count;
}

/*
 * operation write handler
 */
int write_operation(struct file *file, const char *buf, unsigned long count, void *data)
{
	if(count > WRITE_SIZE) {
    		count = WRITE_SIZE;
	}

	//memset(operation, 0, WRITE_SIZE);
	memcpy(operation_input, buf, count);
	return count;
}

/*
 * result read handler
 */
int read_result(char *buffer, char **buffer_location,
				  off_t offset, int buffer_length, int *eof, void *data)
{
	long res = calculate();

	return sprintf(buffer, "%ld\n", res);
}


int init_module()
{
	// parent dir
	//значение NULL если файл находится непосредственно в каталоге /proc
	calc_dir = proc_mkdir(PARENT_DIR, NULL);
	if(!calc_dir) {
		printk(KERN_INFO "Error creating proc entry");
		return -ENOMEM;
	}

	// arg1
	// 0666 - права доступа для файлов
	// 0666 - 0022 = 0644 (что соответствует правам -rw-r--r-- для file)
	// С помощью команды create_proc_entry() в указанной выше поддиректории создается обычный файл "ARG1" с правами доступа 0666.
	arg1 = create_proc_entry(ARG1, 0666, calc_dir);
	if(!arg1) {
	    	printk(KERN_INFO "Error creating proc entry");
	    	return -ENOMEM;
    	}
	//запись данных в виртуальный файл (из пользовательского пространства в ядро)
	arg1->write_proc = write_arg1;

	// arg2
	arg2 = create_proc_entry(ARG2, 0666, calc_dir);
	if(!arg2) {
	    	printk(KERN_INFO "Error creating proc entry");
	    	return -ENOMEM;
    	}
	arg2->write_proc = write_arg2;

	// operation
	operation = create_proc_entry(OPERATION, 0666, calc_dir);
	if(!operation) {
	    	printk(KERN_INFO "Error creating proc entry");
	    	return -ENOMEM;
    	}
	operation->write_proc = write_operation;

	// result
	result = create_proc_entry(RESULT, 0666, calc_dir);
	if(!result) {
	    	printk(KERN_INFO "Error creating proc entry");
	    	return -ENOMEM;
	}
	//считывает данные из виртуального файла (из ядра в пользовательское пространство) 
	result->read_proc = read_result;

	printk(KERN_INFO "/proc/%s created\n", PARENT_DIR);
	return 0;
}

void cleanup_module()
{
	remove_proc_entry(ARG1, NULL);
	remove_proc_entry(ARG2, NULL);
	remove_proc_entry(OPERATION, NULL);
	remove_proc_entry(RESULT, NULL);
	printk(KERN_INFO "/proc/%s removed\n", PARENT_DIR);
}
#endif
