#include <linux/tty.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/list.h>
#include<linux/kernel.h>

#define IF_TRUE_GOTO(expr, label) {							\
	if((expr)) {									\
		printk("error: %s, %s, %d\n",						\
			 __FILE__, __FUNCTION__, __LINE__);				\
		goto label;								\
	}										\
}
#define IF_FALSE_GOTO(expr, label)							\
	IF_TRUE_GOTO(!(expr), label)
#define IF_NULL_GOTO(expr, label)							\
	IF_TRUE_GOTO((expr)==NULL, label)
#define PENTER printk(KERN_INFO "entering %s\n", __FUNCTION__)

#define procfs_MAGIC 0xFEEDBEEF
#define MAX_FILE_SIZE 4096
#define DEFAULT_MODE_FILE	(S_IFREG | 0444)
#define DEFAULT_MODE_DIR	(S_IFDIR | 0555)

typedef struct _task_frame
{
	struct task_struct *task;
	struct dentry *d_entry;
	struct list_head list;
}task_frame;

static struct dentry *procfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data);
static int procfs_create_tree(struct super_block *sb);
static void procfs_destroy_inode(struct inode *inode);
static struct inode *procfs_create_inode(struct super_block *sb);
static struct dentry *procfs_create_dir(struct super_block *sb, struct dentry *dir, char *name);
static struct dentry *procfs_create_file(struct super_block *sb, struct dentry *dir, char *name, char *msg);
static ssize_t procfs_read_file(struct file *file, char *buf, size_t count, loff_t *offset);
static ssize_t procfs_write_file(struct file *file, const char *buf, size_t count, loff_t *offset);
static int procfs_open(struct inode *inode, struct file *file);

static const struct super_operations procfs_super_ops = {
	.statfs = simple_statfs,
	//TODO: initialize destroy_inode field
	.destroy_inode = procfs_destroy_inode,
};
static struct file_operations procfs_file_ops = {
	//TODO: initialize the open, read and write fields
	.open = procfs_open,
	.read = procfs_read_file,
	.write = procfs_write_file,
};


static struct file_system_type procfs_type = {
	.owner = THIS_MODULE,
	.kill_sb = kill_litter_super,
	//TODO:update name and mount fields 
	//name: name of this filesystem 
	//mount: the function that mounts this filesystem
	.name = "procfs",//?
	.mount = procfs_mount,
};

static int __init procfs_init(void) {
	PENTER;
	return register_filesystem(&procfs_type);
}
static void __exit procfs_exit(void) {
	PENTER;
	unregister_filesystem(&procfs_type);
}

MODULE_AUTHOR("Mayukh Maitra");
MODULE_LICENSE("GPL");
module_init(procfs_init);
module_exit(procfs_exit);

static int procfs_fill_super(struct super_block *sb, void *data, int silent) {
	static struct tree_descr files[] = {{""}};
	PENTER;
	IF_TRUE_GOTO(simple_fill_super(sb, procfs_MAGIC, files), fail);
	//TODO: update sb's super_operation element
	sb->s_op = &procfs_super_ops;

	IF_TRUE_GOTO(procfs_create_tree(sb), fail);
	return 0;
fail:
	return -1;
}


static struct dentry *procfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data) {
	PENTER;
	return mount_single(fs_type, flags, data, procfs_fill_super);
}

//create tree function
static int procfs_create_tree(struct super_block *sb) {
	struct dentry *dir = sb->s_root;
	struct dentry *next_dir;
	char *temporary_file_string = kmalloc(sizeof(char)*4096, GFP_KERNEL);
	char *temporary_directory_name = kmalloc(sizeof(char)*20, GFP_KERNEL);
	char *temporary_Tty_name = kmalloc(sizeof(char)*20, GFP_KERNEL);
	task_frame *List_node;
	LIST_HEAD(head);
	struct list_head *pos;
	PENTER;

	List_node = kmalloc(sizeof(task_frame), GFP_KERNEL);
	List_node->task = &init_task;
	List_node->d_entry = dir;
	INIT_LIST_HEAD(&List_node->list);
	list_add(&List_node->list, &head);

	while(!list_empty(head.next))
	{
		List_node = list_first_entry(&head, typeof(*List_node), list);
		if(List_node->task && List_node->task->signal && List_node->task->signal->tty)
			temporary_Tty_name = List_node->task->signal->tty->name;
		else
			temporary_Tty_name = "NA";
		sprintf(temporary_file_string, "process name: %s\npid: %d\ntty name: %s\n", List_node->task->comm, List_node->task->pid, temporary_Tty_name);
		IF_NULL_GOTO(procfs_create_file(sb, List_node->d_entry, "proc_info.txt", temporary_file_string), fail);
		if(!list_empty(&List_node->list))
			list_del(&List_node->list);
		list_for_each(pos, &List_node->task->children)
		{
			task_frame *temp = kmalloc(sizeof(task_frame), GFP_KERNEL);
			struct task_struct *child;
			child = list_entry(pos, struct task_struct, sibling);
			temp->task = child;
			INIT_LIST_HEAD(&temp->list);
			list_add(&temp->list, &head);
			sprintf(temporary_directory_name, "%s_%d", child->comm, child->pid);
			IF_NULL_GOTO(next_dir = procfs_create_dir(sb, List_node->d_entry, temporary_directory_name), fail);
			temp->d_entry = next_dir;
			kfree(temp);
		}
	
	}
	;	
	return 0;
fail:
	return -1;
}

static void procfs_destroy_inode(struct inode *inode) {
	PENTER;
	//TODO: for a regular file, free it's i_private field
	kfree(inode->i_private);
}
//create inode function
static struct inode *procfs_create_inode(struct super_block *sb)
{
	struct inode *inode = new_inode(sb);
	PENTER;
	IF_NULL_GOTO(inode, fail)

	//TODO: initialize, i_ino, i_uid, i_gid, i_atime, i_mtime, i_ctime fields 
	// using get_next_ino(), current_fsuid(), current_fsgid(), and 
	// CURRENT_TIME 
	inode->i_ino = get_next_ino();
	inode->i_uid = current_fsuid();
	inode->i_gid = current_fsgid();
	inode->i_atime = CURRENT_TIME;
	inode->i_mtime = CURRENT_TIME;
	inode->i_ctime = CURRENT_TIME;

	return inode;
fail:
	return NULL;
} 

//create directory function
static struct dentry *procfs_create_dir(struct super_block *sb, struct dentry *dir, char *name) {
	struct dentry *dentry = NULL;
	struct inode *inode = NULL;

	PENTER;
	IF_NULL_GOTO(dentry = d_alloc_name(dir, name), fail);
	IF_NULL_GOTO(inode = procfs_create_inode(sb), fail);
	
	//TODO: update i_mode, i_size(= 64), i_op, and i_fop fields 
	// using DEFAULT_MODE_DIR, simple_dir_inode_operations and 
	// simple_dir_operations 	
	inode->i_mode = DEFAULT_MODE_DIR;
	inode->i_size = 64;
	inode->i_op = &simple_dir_inode_operations;
	inode->i_fop = &simple_dir_operations;


	d_add(dentry, inode);
	return dentry;
fail:
	if(dentry)
		dput(dentry);
	if(inode)
		iput(inode);
	return NULL;
}

//create file function
static struct dentry *procfs_create_file(struct super_block *sb, struct dentry *dir, char *name, char *msg) {
	struct dentry *dentry = NULL;
	struct inode *inode = NULL;

	PENTER;
	IF_NULL_GOTO(dentry = d_alloc_name(dir, name), fail);
	IF_NULL_GOTO(inode = procfs_create_inode(sb), fail);
	//TODO: update i_mode, i_size(=strlen(msg)), and i_fop fields 
	// allocate MAX_FILE_SIZE buffer and make i_private pointing to it, 
 	// copy the contents of msg to i_private, 
	// add inode to dentry
	inode->i_mode = DEFAULT_MODE_FILE;
	inode->i_size = strlen(msg);
	inode->i_fop = &procfs_file_ops;

	inode->i_private = kmalloc(sizeof(char)*MAX_FILE_SIZE, GFP_KERNEL);
	strcpy(inode->i_private, msg);
	d_add(dentry, inode);

	return dentry;
fail:
	if(dentry)
		dput(dentry);
	if(inode && inode->i_private)
		kfree(inode->i_private);
	if(inode)
		iput(inode);
	return NULL;
}

//read file function
static ssize_t procfs_read_file(struct file *file, char *buf, size_t count, loff_t *offset) {
	char *msg = file->private_data; // = TODO: we will read from file's private_data
	long msglen = strlen(msg);

	PENTER;
	return simple_read_from_buffer(buf, count, offset, msg, msglen);
}
//write file function
static ssize_t procfs_write_file(struct file *file, const char *buf, size_t count, loff_t *offset) {
	char *msg = file->private_data;// = TODO: we will write to file's private_data
	ssize_t ret = 0;

	PENTER;
	ret = simple_write_to_buffer(msg, MAX_FILE_SIZE-1, offset, buf, count);
	if(ret >= 0){
		file->f_inode->i_size = ret;
		msg[ret] = 0;
	}
	return ret;
}

static int procfs_open(struct inode *inode, struct file *file){
	PENTER;
	//TODO: make file’s private_data pointing to the buffer 
	// allocated when creating the file 
	file->private_data = inode->i_private;

	return 0;


}


