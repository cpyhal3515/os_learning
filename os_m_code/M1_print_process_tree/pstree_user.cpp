#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <cassert>
#include <dirent.h>
#include <cstdio>
using namespace std;

#define IN_PARAM_LOC 1
#define PSTREE_USER_VERSION "1.0"
#define INFROM_BUF_LEN 50
#define FILE_PATH_STR_LEN 30
#define INFORM_STRUCT_NUM 1000

// 定义输入参数的枚举类型
enum PARAM {PIDS, VERSION};

// 定义信息 struct
struct Inform_struct 
{
  string name;
  int   self_id;
  int   father_id;
} inform_list[INFORM_STRUCT_NUM];

// 建立 pid 号到名称 name 的映射
map<int, string> pid2name;

// 定义树的节点
struct Process_node 
{
  int pid;
  vector<Process_node*> child_node_list;
};

// 函数声明
void action_for_diff_param (enum PARAM param);
void get_proc_list(void);
struct Process_node* construct_pid_tree(void);
void printf_pid_tree(struct Process_node* pid_tree, string & printf_string, enum PARAM param);

int process_num = 0;
int main(int argc, char *argv[]) 
{
  enum PARAM input_param;
  // 完成输入参数的解析
  assert(argv[IN_PARAM_LOC]); 
  if(string(argv[IN_PARAM_LOC]) == "-p" || string(argv[IN_PARAM_LOC]) == "--show-pids")
    input_param = PIDS;
  else if(string(argv[IN_PARAM_LOC]) == "-V" || string(argv[IN_PARAM_LOC]) == "--version")
    input_param = VERSION;
  else
  {
    cout << "Parameter error: Only -p(--show-pids), -V(--version)" << endl;
    return -1;
  }
  assert(!argv[argc]); 

  action_for_diff_param (input_param);

  return 0;
}


/*
 * action_for_diff_param - 根据不同的参数执行不同的操作
* @param1 enum PARAM param - 输入参数
* @return
*/
void action_for_diff_param (enum PARAM param) 
{
  if(param == PIDS)
  {
    get_proc_list();
    struct Process_node* root = construct_pid_tree();
    string blank = "";
    printf_pid_tree(root, blank, param);
  }
  else if(param == VERSION)
  {
    cout << "pstree_user-64 version " << PSTREE_USER_VERSION << endl;
  }
  return;
}


/*
 * get_proc_list - 获得进程列表
* @param1
* @return
*/
void get_proc_list(void)
{
  DIR* proc_dir = opendir("/proc");
  if (proc_dir == NULL) 
  {
      perror("Error opening /proc directory");
      exit(1);
  }

  int process_num_cnt = 0;
  struct dirent* entry;
  while ((entry = readdir(proc_dir)) != NULL) 
  {
      if (entry->d_type == DT_DIR) 
      {
          // 检查目录名称是否是有效的 Process ID
          int pid = atoi(entry->d_name);
          if (pid > 0) 
          {
            // FILE* ptr;
            string path_buf = "/proc/" + to_string(pid) + "/status";

            // 打开文件，读模式
            string line;
            ifstream pid_file(path_buf);
            if (pid_file.is_open()) 
            {
                while (getline(pid_file, line)) 
                {
                    if(line.substr(0, sizeof("Name") - 1) == "Name")
                    {
                      inform_list[process_num_cnt].name = line.substr(sizeof("Name") + 1, line.size() - sizeof("Name"));
                    }
                    else if(line.substr(0, sizeof("Pid") - 1) == "Pid")
                    {
                      inform_list[process_num_cnt].self_id = stoi(line.substr(sizeof("Pid") + 1, line.size() - sizeof("Pid")));
                      pid2name[stoi(line.substr(sizeof("Pid") + 1, line.size() - sizeof("Pid")))] = inform_list[process_num_cnt].name;
                    }
                    else if(line.substr(0, sizeof("PPid") - 1) == "PPid")
                    {
                      inform_list[process_num_cnt].father_id = stoi(line.substr(sizeof("PPid") + 1, line.size() - sizeof("PPid")));
                      break;
                    }
                }
                process_num_cnt++;
                pid_file.close();
            }
            else 
            {
                cout << "Unable to open file" << endl;
            }
          }
      }
  }
  process_num = process_num_cnt;
  closedir(proc_dir);
}

/*
 * construct_pid_tree - 根据进程列表建立一棵树
* @param1
* @return struct Process_node* - 返回树的根节点
*/
struct Process_node* construct_pid_tree(void)
{
  // 初始化一个队列
  queue<Process_node*> tree_node_queue;
  // 新建根节点
  struct Process_node* root = new Process_node;
  root->pid = 1;
  // 将根节点入队
  tree_node_queue.push(root);
  
  while (!tree_node_queue.empty())
  {
    // 每次取出一个节点
    struct Process_node* cur_node = tree_node_queue.front();
    tree_node_queue.pop();
    // 查找其孩子节点
    for (int i = 0; i < process_num; i++)
    {
        if(inform_list[i].father_id == cur_node->pid)
        {
          struct Process_node* node = new Process_node;
          node->pid = inform_list[i].self_id;
          tree_node_queue.push(node);
          cur_node->child_node_list.push_back(node);
        }
    }
  }
  return root;
  
}

/*
 * printf_pid_tree - 根据建立的树将其打印出来
* @param1 struct Process_node* pid_tree - 建立的树
* @param2 string & printf_string - 打印的拼接字符串
* @param3 enum PARAM param - 输入的参数
* @return 
*/
void printf_pid_tree(struct Process_node* pid_tree, string & printf_string, enum PARAM param)
{
  // 递归结束
  if(pid_tree->child_node_list.empty())
  {
    cout << printf_string + "+--" + pid2name[pid_tree->pid] + '(' + to_string(pid_tree->pid) + ')' << endl;
    return;
  }

  unsigned int this_string_part_len;
  this_string_part_len = sizeof("+--") + pid2name[pid_tree->pid].size() + 2 + to_string(pid_tree->pid).size() - 1;
  printf_string = printf_string + "+--" + pid2name[pid_tree->pid] + '(' + to_string(pid_tree->pid) + ')';
    
  for(unsigned int i = 0; i < pid_tree->child_node_list.size(); i++)
  {
    // 当 i > 0 时在 + 前面增加 |
    if(i > 0)
    {
      printf_string = printf_string.replace(0, printf_string.size(), string(printf_string.size(), ' '));
      printf_string[printf_string.size() - 1] = '|';
    }
    // 递归调用
    printf_pid_tree(pid_tree->child_node_list[i], printf_string, param);
  }
  printf_string.resize(printf_string.size() - this_string_part_len);

}
