/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2023. All rights reserved.
 * Description: 华为软件精英挑战赛初赛练习赛，C/C++链接多线程临时解决方案代码
 * Author: 张元龙
 * Date: 2023-03-14
 * Note: 
 *   由于一个个人小失误，导致初赛练习赛的CMakeLists.txt没能-lpthread，由于平台涉及到数千节点的变更且练习赛正在运行，变更风险极高，故通过平台变更去修改CMakeLists.txt极为困难
 *   这个失误耗掉了我一天的时间思考如何补救，终于想到了不用变更平台的解决方案并将它实现了下来
 *   用法: 直接将这个.cpp 放到项目中一起编译即可在不添加 -lpthread 链接选项的时候使用多线程。
 *   注意：改代码仍处于实验阶段，仅用于初赛的练习赛，从初赛正式赛开始，将不再使用。
 *   工作原理：
 *     1. 在main函数开始前，设置环境变量 LD_PRELOAD=libpthread.so.0， 然后execvp重启自身进程
 *     2. 由于环境变量 LD_PRELOAD 的作用，进程在重启之后会自动加载 libpthread.so 库
 *     3. 搜索/proc/self/maps，从中找到 libpthread.so 的加载地址和路径
 *     4. 解析libpthread.so的ELF格式，搜索所有pthread_系列的函数地址，并将这些地址填入一个数组表中：g_pthread_func_table
 *     5. 基于汇编实现所有 pthread_系列函数，用于链接, 此时所有针对pthread函数的调用都是调用的这些汇编函数
 *     6. 这些汇编函数会通过查表来找到真正的pthread函数在libpthread.so的地址 ，并通过jmp指令 保留所有参数/环境的情况下跳转过去
 *     7. 如果一个pthread_函数在函数表初始化之前被调用，那么将自动触发函数表的更新
 *   更新日志：
 *     2023-03-15：支持libpthread.so中带多个同名函数时，指向最新版本
 */

// 此代码仅用于初赛练习赛，从正式赛开始，cmake将加上-lpthread，同时加上DISABLE_PTHREAD_HOOK定义，以确保本代码不会再被编译进去
#if defined(__linux) && !defined(DISABLE_PTHREAD_HOOK)  // DISABLE_PTHREAD_HOOK 用于后续关闭的开关
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <assert.h>
#include <string.h>
#include <linux/limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>
#include <sys/mman.h>
#include <vector>
using namespace std;

static_assert(sizeof(void*) == 8, "Only support 64bits system.");   // 只适用于64位系统

// pthread 函数表，这些名字已经经过排序, 用于后续的二分查找
static const char * g_pthread_func_name[] = {
    "pthread_atfork", "pthread_attr_getaffinity_np", "pthread_attr_getguardsize", "pthread_attr_getschedpolicy", "pthread_attr_getscope", "pthread_attr_getstack", "pthread_attr_getstackaddr", "pthread_attr_getstacksize", "pthread_attr_setaffinity_np", "pthread_attr_setguardsize", 
    "pthread_attr_setschedpolicy", "pthread_attr_setscope", "pthread_attr_setstack", "pthread_attr_setstackaddr", "pthread_attr_setstacksize", "pthread_barrier_destroy", "pthread_barrier_init", "pthread_barrier_wait", "pthread_barrierattr_destroy", "pthread_barrierattr_getpshared", 
    "pthread_barrierattr_init", "pthread_barrierattr_setpshared", "pthread_cancel", "pthread_clockjoin_np", "pthread_cond_broadcast", "pthread_cond_clockwait", "pthread_cond_destroy", "pthread_cond_init", "pthread_cond_signal", "pthread_cond_timedwait", 
    "pthread_cond_wait", "pthread_condattr_destroy", "pthread_condattr_getclock", "pthread_condattr_getpshared", "pthread_condattr_init", "pthread_condattr_setclock", "pthread_condattr_setpshared", "pthread_create", "pthread_detach", "pthread_exit", 
    "pthread_getaffinity_np", "pthread_getattr_default_np", "pthread_getattr_np", "pthread_getconcurrency", "pthread_getcpuclockid", "pthread_getname_np", "pthread_getschedparam", "pthread_getspecific", "pthread_join", "pthread_key_create", 
    "pthread_key_delete", "pthread_kill", "pthread_kill_other_threads_np", "pthread_mutex_clocklock", "pthread_mutex_consistent", "pthread_mutex_consistent_np", "pthread_mutex_destroy", "pthread_mutex_getprioceiling", "pthread_mutex_init", "pthread_mutex_lock", 
    "pthread_mutex_setprioceiling", "pthread_mutex_timedlock", "pthread_mutex_trylock", "pthread_mutex_unlock", "pthread_mutexattr_destroy", "pthread_mutexattr_getkind_np", "pthread_mutexattr_getprioceiling", "pthread_mutexattr_getprotocol", "pthread_mutexattr_getpshared", "pthread_mutexattr_getrobust", 
    "pthread_mutexattr_getrobust_np", "pthread_mutexattr_gettype", "pthread_mutexattr_init", "pthread_mutexattr_setkind_np", "pthread_mutexattr_setprioceiling", "pthread_mutexattr_setprotocol", "pthread_mutexattr_setpshared", "pthread_mutexattr_setrobust", "pthread_mutexattr_setrobust_np", "pthread_mutexattr_settype", 
    "pthread_once", "pthread_rwlock_clockrdlock", "pthread_rwlock_clockwrlock", "pthread_rwlock_destroy", "pthread_rwlock_init", "pthread_rwlock_rdlock", "pthread_rwlock_timedrdlock", "pthread_rwlock_timedwrlock", "pthread_rwlock_tryrdlock", "pthread_rwlock_trywrlock", 
    "pthread_rwlock_unlock", "pthread_rwlock_wrlock", "pthread_rwlockattr_destroy", "pthread_rwlockattr_getkind_np", "pthread_rwlockattr_getpshared", "pthread_rwlockattr_init", "pthread_rwlockattr_setkind_np", "pthread_rwlockattr_setpshared", "pthread_setaffinity_np", "pthread_setattr_default_np", 
    "pthread_setcancelstate", "pthread_setcanceltype", "pthread_setconcurrency", "pthread_setname_np", "pthread_setschedparam", "pthread_setschedprio", "pthread_setspecific", "pthread_sigmask", "pthread_sigqueue", "pthread_spin_destroy", 
    "pthread_spin_init", "pthread_spin_lock", "pthread_spin_trylock", "pthread_spin_unlock", "pthread_testcancel", "pthread_timedjoin_np", "pthread_tryjoin_np", "pthread_yield", 
};

constexpr int NAME_TOT = sizeof (g_pthread_func_name) / sizeof(g_pthread_func_name[0]);

extern const void * g_pthread_func_table[NAME_TOT];
const void * g_pthread_func_table[NAME_TOT];

// 把函数名通过二分查找映射为g_pthread_func_table的下标
inline int GetFuncId(const char *name)
{
    int l = 0;
    int r = NAME_TOT;
    while (l < r) {
        int m = (l + r) / 2;
        int cmp = strcmp(name, g_pthread_func_name[m]);
        if (cmp < 0) {
            r = m;
        }
        else if (cmp > 0) {
            l = m + 1;
        }
        else {
            return m;
        }
    }
    return l < NAME_TOT && strcmp(name, g_pthread_func_name[l]) == 0 ? l : -1;
}

// 从ELF中搜索pthread系列函数地址, 并写入g_pthread_func_table表中
void ReadElf(const char *mem, const char *baseAddr)
{
    assert(*(uint32_t*)mem == 0x464c457f);  // .ELF

    Elf64_Ehdr *header = (Elf64_Ehdr*)mem;
    
    assert(header->e_shentsize == sizeof(Elf64_Shdr));
    
    const char *symboStrTable = nullptr;

    Elf64_Shdr* secHeader = (Elf64_Shdr*)(mem + header->e_shoff);
    const char *headerStr = mem + secHeader[header->e_shstrndx].sh_offset;
    for (int i = 0; i < header->e_shnum; i++) {
        if (strcmp(headerStr + secHeader[i].sh_name, ".strtab") == 0) {
            symboStrTable = mem + secHeader[i].sh_offset;
            break;
        }
    }
    assert(symboStrTable);
    const void * local_pthread_func_table[NAME_TOT] = {0};
    uint32_t ver[NAME_TOT] = {0};

    for (int i = 0; i < header->e_shnum; i++) {

        if (secHeader[i].sh_type == SHT_SYMTAB) {
            int j = 0;
            Elf64_Sym *symHeader = (Elf64_Sym*)(mem + secHeader[i].sh_offset);
            for (;(char*)(symHeader) < mem + secHeader[i].sh_offset + secHeader[i].sh_size; symHeader++,j++) {
                if (symHeader->st_name == 0 || symHeader->st_value == 0) continue;
                if (ELF64_ST_TYPE(symHeader->st_info) !=STT_FUNC || (ELF64_ST_BIND(symHeader->st_info) != STB_GLOBAL && ELF64_ST_BIND(symHeader->st_info) != STB_WEAK)) {
                    continue;
                }
                const char *name = symboStrTable + symHeader->st_name;
                if (memcmp(name, "pthread_", 8) == 0) {
                    char buf[128];
                    uint32_t v = 0;
                    strncpy(buf, name, sizeof buf);
                    char *p = strchr(buf, '@');
                    if (p) {
                        *p++ = 0;
                        p = strstr(p, "GLIBC_");
                        if (p) {
                            p += 6;
                            char *end;
                            v = strtoul(p, &end, 10);
                            //assert(v == 2);
                            v *= 100;
                            //assert(*end == '.');
                            p = end + 1;
                            v += strtoul(p, &end, 10);
                            v *= 100;
                            //assert(*end == '.');
                            p = end + 1;
                            v += strtoul(p, &end, 10);
                            //assert(*end == 0);
                        }
                    } 

                    int id = GetFuncId(buf);
                    if (id != -1 && (local_pthread_func_table[id] == nullptr || v > ver[id])) {
                        local_pthread_func_table[id] = baseAddr + symHeader->st_value;
                        ver[id] = v;
                    }
                }
                
            }

        }
    }
    memcpy(g_pthread_func_table, local_pthread_func_table, sizeof g_pthread_func_table);

}

void ReadPthreadLibFile(const char* fileName, const char *baseAddr)
{
    int fd = open(fileName, O_RDONLY);
    assert(fd >= 0);

    struct stat statbuf;
    fstat(fd, &statbuf);

    void *addr = mmap(nullptr, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
    assert(addr != (void *) -1);
    close(fd);

    ReadElf((const char*)addr, baseAddr);
    munmap(addr, statbuf.st_size);
}

// 查找libpthread.so在内存中的加载地址
void *GetPthreadBaseAddr(char fileName[PATH_MAX]) 
{
    // 查找pthread加载基址
    char buf[1024 * 64];

    int fd = open("/proc/self/maps", O_RDONLY);
    assert(fd >= 0);
    int ret = read(fd, buf, sizeof buf);
    assert(ret > 0 && ret < sizeof buf);
    close(fd);
    buf[ret] = '\n';
    char *p = strstr(buf, "libpthread");

    if (p == nullptr) {
        return nullptr;
    }

    char *end = strchr(p, '\n');
    assert(end);
    *end = 0;

    while (!isblank(*p)) p--;
    p++;

    strncpy(fileName, p, PATH_MAX);
    while (*p != '\n' && p >= buf) --p;
    p++;
    end = strchr(p, '-');
    assert(end);
    *end = 0;
    return (void*)strtoull(p, nullptr, 16);
}

// 重启自身进程
void RestartProcess()
{
    vector<char *> cmdline;
    FILE *fp = fopen("/proc/self/cmdline", "r");

    char buf[1024 * 16];
    int readed = fread(buf, 1, sizeof buf, fp);
    assert(readed < (int)sizeof buf);
    fclose(fp);

    char* p = buf;
    while (p < buf + readed) {
        while (p < buf + readed && *p == 0) p++;
        if (p < buf + readed) {
            cmdline.push_back((p));
            p += strlen(p) + 1;
        }
    }
    assert(!cmdline.empty());
    cmdline.push_back(nullptr);
    execvp(cmdline[0], &cmdline[0]);
    perror("execvp err:");
    abort();
}

extern "C"
void __attribute__ ((constructor)) 
InitLoadPthread() 
{
    static bool s_inited = false;
    if (s_inited) {
        return ;
    }
    char fileName[PATH_MAX];

    void* pthreadAddr = GetPthreadBaseAddr(fileName);

    if (pthreadAddr == nullptr) {
        char *env = getenv("LD_PRELOAD");
        assert(env == nullptr || strstr(env, "libpthread") == 0);

        string newEnv;
        if (env) {
            newEnv = env;
            newEnv.push_back(';');
        }
        newEnv += "libpthread.so.0";
        setenv("LD_PRELOAD", newEnv.c_str(), 1);
        RestartProcess();
    }
    ReadPthreadLibFile(fileName, (const char*)pthreadAddr);
    s_inited = true;
    //puts("OK");
}


// 以下汇编函数用于实现各个 pthread 函数，并通过查表跳到真实地址
asm(R"(
    .text
    .weak   pthread_atfork

    .type   pthread_atfork, @function
pthread_atfork:
    mov $0, %eax
    jmp .LRealJump

    .weak   pthread_attr_getaffinity_np
    .type   pthread_attr_getaffinity_np, @function
pthread_attr_getaffinity_np:
    mov $1, %eax
    jmp .LRealJump

    .weak   pthread_attr_getguardsize
    .type   pthread_attr_getguardsize, @function
pthread_attr_getguardsize:
    mov $2, %eax
    jmp .LRealJump

    .weak   pthread_attr_getschedpolicy
    .type   pthread_attr_getschedpolicy, @function
pthread_attr_getschedpolicy:
    mov $3, %eax
    jmp .LRealJump

    .weak   pthread_attr_getscope
    .type   pthread_attr_getscope, @function
pthread_attr_getscope:
    mov $4, %eax
    jmp .LRealJump

    .weak   pthread_attr_getstack
    .type   pthread_attr_getstack, @function
pthread_attr_getstack:
    mov $5, %eax
    jmp .LRealJump

    .weak   pthread_attr_getstackaddr
    .type   pthread_attr_getstackaddr, @function
pthread_attr_getstackaddr:
    mov $6, %eax
    jmp .LRealJump

    .weak   pthread_attr_getstacksize
    .type   pthread_attr_getstacksize, @function
pthread_attr_getstacksize:
    mov $7, %eax
    jmp .LRealJump

    .weak   pthread_attr_setaffinity_np
    .type   pthread_attr_setaffinity_np, @function
pthread_attr_setaffinity_np:
    mov $8, %eax
    jmp .LRealJump

    .weak   pthread_attr_setguardsize
    .type   pthread_attr_setguardsize, @function
pthread_attr_setguardsize:
    mov $9, %eax
    jmp .LRealJump


    .weak   pthread_attr_setschedpolicy
    .type   pthread_attr_setschedpolicy, @function
pthread_attr_setschedpolicy:
    mov $10, %eax
    jmp .LRealJump

    .weak   pthread_attr_setscope
    .type   pthread_attr_setscope, @function
pthread_attr_setscope:
    mov $11, %eax
    jmp .LRealJump

    .weak   pthread_attr_setstack
    .type   pthread_attr_setstack, @function
pthread_attr_setstack:
    mov $12, %eax
    jmp .LRealJump

    .weak   pthread_attr_setstackaddr
    .type   pthread_attr_setstackaddr, @function
pthread_attr_setstackaddr:
    mov $13, %eax
    jmp .LRealJump

    .weak   pthread_attr_setstacksize
    .type   pthread_attr_setstacksize, @function
pthread_attr_setstacksize:
    mov $14, %eax
    jmp .LRealJump

    .weak   pthread_barrier_destroy
    .type   pthread_barrier_destroy, @function
pthread_barrier_destroy:
    mov $15, %eax
    jmp .LRealJump

    .weak   pthread_barrier_init
    .type   pthread_barrier_init, @function
pthread_barrier_init:
    mov $16, %eax
    jmp .LRealJump

    .weak   pthread_barrier_wait
    .type   pthread_barrier_wait, @function
pthread_barrier_wait:
    mov $17, %eax
    jmp .LRealJump

    .weak   pthread_barrierattr_destroy
    .type   pthread_barrierattr_destroy, @function
pthread_barrierattr_destroy:
    mov $18, %eax
    jmp .LRealJump

    .weak   pthread_barrierattr_getpshared
    .type   pthread_barrierattr_getpshared, @function
pthread_barrierattr_getpshared:
    mov $19, %eax
    jmp .LRealJump


    .weak   pthread_barrierattr_init
    .type   pthread_barrierattr_init, @function
pthread_barrierattr_init:
    mov $20, %eax
    jmp .LRealJump

    .weak   pthread_barrierattr_setpshared
    .type   pthread_barrierattr_setpshared, @function
pthread_barrierattr_setpshared:
    mov $21, %eax
    jmp .LRealJump

    .weak   pthread_cancel
    .type   pthread_cancel, @function
pthread_cancel:
    mov $22, %eax
    jmp .LRealJump

    .weak   pthread_clockjoin_np
    .type   pthread_clockjoin_np, @function
pthread_clockjoin_np:
    mov $23, %eax
    jmp .LRealJump

    .weak   pthread_cond_broadcast
    .type   pthread_cond_broadcast, @function
pthread_cond_broadcast:
    mov $24, %eax
    jmp .LRealJump

    .weak   pthread_cond_clockwait
    .type   pthread_cond_clockwait, @function
pthread_cond_clockwait:
    mov $25, %eax
    jmp .LRealJump

    .weak   pthread_cond_destroy
    .type   pthread_cond_destroy, @function
pthread_cond_destroy:
    mov $26, %eax
    jmp .LRealJump

    .weak   pthread_cond_init
    .type   pthread_cond_init, @function
pthread_cond_init:
    mov $27, %eax
    jmp .LRealJump

    .weak   pthread_cond_signal
    .type   pthread_cond_signal, @function
pthread_cond_signal:
    mov $28, %eax
    jmp .LRealJump

    .weak   pthread_cond_timedwait
    .type   pthread_cond_timedwait, @function
pthread_cond_timedwait:
    mov $29, %eax
    jmp .LRealJump


    .weak   pthread_cond_wait
    .type   pthread_cond_wait, @function
pthread_cond_wait:
    mov $30, %eax
    jmp .LRealJump

    .weak   pthread_condattr_destroy
    .type   pthread_condattr_destroy, @function
pthread_condattr_destroy:
    mov $31, %eax
    jmp .LRealJump

    .weak   pthread_condattr_getclock
    .type   pthread_condattr_getclock, @function
pthread_condattr_getclock:
    mov $32, %eax
    jmp .LRealJump

    .weak   pthread_condattr_getpshared
    .type   pthread_condattr_getpshared, @function
pthread_condattr_getpshared:
    mov $33, %eax
    jmp .LRealJump

    .weak   pthread_condattr_init
    .type   pthread_condattr_init, @function
pthread_condattr_init:
    mov $34, %eax
    jmp .LRealJump

    .weak   pthread_condattr_setclock
    .type   pthread_condattr_setclock, @function
pthread_condattr_setclock:
    mov $35, %eax
    jmp .LRealJump

    .weak   pthread_condattr_setpshared
    .type   pthread_condattr_setpshared, @function
pthread_condattr_setpshared:
    mov $36, %eax
    jmp .LRealJump

    .weak   pthread_create
    .type   pthread_create, @function
pthread_create:
    mov $37, %eax
    jmp .LRealJump

    .weak   pthread_detach
    .type   pthread_detach, @function
pthread_detach:
    mov $38, %eax
    jmp .LRealJump

    .weak   pthread_exit
    .type   pthread_exit, @function
pthread_exit:
    mov $39, %eax
    jmp .LRealJump


    .weak   pthread_getaffinity_np
    .type   pthread_getaffinity_np, @function
pthread_getaffinity_np:
    mov $40, %eax
    jmp .LRealJump

    .weak   pthread_getattr_default_np
    .type   pthread_getattr_default_np, @function
pthread_getattr_default_np:
    mov $41, %eax
    jmp .LRealJump

    .weak   pthread_getattr_np
    .type   pthread_getattr_np, @function
pthread_getattr_np:
    mov $42, %eax
    jmp .LRealJump

    .weak   pthread_getconcurrency
    .type   pthread_getconcurrency, @function
pthread_getconcurrency:
    mov $43, %eax
    jmp .LRealJump

    .weak   pthread_getcpuclockid
    .type   pthread_getcpuclockid, @function
pthread_getcpuclockid:
    mov $44, %eax
    jmp .LRealJump

    .weak   pthread_getname_np
    .type   pthread_getname_np, @function
pthread_getname_np:
    mov $45, %eax
    jmp .LRealJump

    .weak   pthread_getschedparam
    .type   pthread_getschedparam, @function
pthread_getschedparam:
    mov $46, %eax
    jmp .LRealJump

    .weak   pthread_getspecific
    .type   pthread_getspecific, @function
pthread_getspecific:
    mov $47, %eax
    jmp .LRealJump

    .weak   pthread_join
    .type   pthread_join, @function
pthread_join:
    mov $48, %eax
    jmp .LRealJump

    .weak   pthread_key_create
    .type   pthread_key_create, @function
pthread_key_create:
    mov $49, %eax
    jmp .LRealJump


    .weak   pthread_key_delete
    .type   pthread_key_delete, @function
pthread_key_delete:
    mov $50, %eax
    jmp .LRealJump

    .weak   pthread_kill
    .type   pthread_kill, @function
pthread_kill:
    mov $51, %eax
    jmp .LRealJump

    .weak   pthread_kill_other_threads_np
    .type   pthread_kill_other_threads_np, @function
pthread_kill_other_threads_np:
    mov $52, %eax
    jmp .LRealJump

    .weak   pthread_mutex_clocklock
    .type   pthread_mutex_clocklock, @function
pthread_mutex_clocklock:
    mov $53, %eax
    jmp .LRealJump

    .weak   pthread_mutex_consistent
    .type   pthread_mutex_consistent, @function
pthread_mutex_consistent:
    mov $54, %eax
    jmp .LRealJump

    .weak   pthread_mutex_consistent_np
    .type   pthread_mutex_consistent_np, @function
pthread_mutex_consistent_np:
    mov $55, %eax
    jmp .LRealJump

    .weak   pthread_mutex_destroy
    .type   pthread_mutex_destroy, @function
pthread_mutex_destroy:
    mov $56, %eax
    jmp .LRealJump

    .weak   pthread_mutex_getprioceiling
    .type   pthread_mutex_getprioceiling, @function
pthread_mutex_getprioceiling:
    mov $57, %eax
    jmp .LRealJump

    .weak   pthread_mutex_init
    .type   pthread_mutex_init, @function
pthread_mutex_init:
    mov $58, %eax
    jmp .LRealJump

    .weak   pthread_mutex_lock
    .type   pthread_mutex_lock, @function
pthread_mutex_lock:
    mov $59, %eax
    jmp .LRealJump


    .weak   pthread_mutex_setprioceiling
    .type   pthread_mutex_setprioceiling, @function
pthread_mutex_setprioceiling:
    mov $60, %eax
    jmp .LRealJump

    .weak   pthread_mutex_timedlock
    .type   pthread_mutex_timedlock, @function
pthread_mutex_timedlock:
    mov $61, %eax
    jmp .LRealJump

    .weak   pthread_mutex_trylock
    .type   pthread_mutex_trylock, @function
pthread_mutex_trylock:
    mov $62, %eax
    jmp .LRealJump

    .weak   pthread_mutex_unlock
    .type   pthread_mutex_unlock, @function
pthread_mutex_unlock:
    mov $63, %eax
    jmp .LRealJump

    .weak   pthread_mutexattr_destroy
    .type   pthread_mutexattr_destroy, @function
pthread_mutexattr_destroy:
    mov $64, %eax
    jmp .LRealJump

    .weak   pthread_mutexattr_getkind_np
    .type   pthread_mutexattr_getkind_np, @function
pthread_mutexattr_getkind_np:
    mov $65, %eax
    jmp .LRealJump

    .weak   pthread_mutexattr_getprioceiling
    .type   pthread_mutexattr_getprioceiling, @function
pthread_mutexattr_getprioceiling:
    mov $66, %eax
    jmp .LRealJump

    .weak   pthread_mutexattr_getprotocol
    .type   pthread_mutexattr_getprotocol, @function
pthread_mutexattr_getprotocol:
    mov $67, %eax
    jmp .LRealJump

    .weak   pthread_mutexattr_getpshared
    .type   pthread_mutexattr_getpshared, @function
pthread_mutexattr_getpshared:
    mov $68, %eax
    jmp .LRealJump

    .weak   pthread_mutexattr_getrobust
    .type   pthread_mutexattr_getrobust, @function
pthread_mutexattr_getrobust:
    mov $69, %eax
    jmp .LRealJump


    .weak   pthread_mutexattr_getrobust_np
    .type   pthread_mutexattr_getrobust_np, @function
pthread_mutexattr_getrobust_np:
    mov $70, %eax
    jmp .LRealJump

    .weak   pthread_mutexattr_gettype
    .type   pthread_mutexattr_gettype, @function
pthread_mutexattr_gettype:
    mov $71, %eax
    jmp .LRealJump

    .weak   pthread_mutexattr_init
    .type   pthread_mutexattr_init, @function
pthread_mutexattr_init:
    mov $72, %eax
    jmp .LRealJump

    .weak   pthread_mutexattr_setkind_np
    .type   pthread_mutexattr_setkind_np, @function
pthread_mutexattr_setkind_np:
    mov $73, %eax
    jmp .LRealJump

    .weak   pthread_mutexattr_setprioceiling
    .type   pthread_mutexattr_setprioceiling, @function
pthread_mutexattr_setprioceiling:
    mov $74, %eax
    jmp .LRealJump

    .weak   pthread_mutexattr_setprotocol
    .type   pthread_mutexattr_setprotocol, @function
pthread_mutexattr_setprotocol:
    mov $75, %eax
    jmp .LRealJump

    .weak   pthread_mutexattr_setpshared
    .type   pthread_mutexattr_setpshared, @function
pthread_mutexattr_setpshared:
    mov $76, %eax
    jmp .LRealJump

    .weak   pthread_mutexattr_setrobust
    .type   pthread_mutexattr_setrobust, @function
pthread_mutexattr_setrobust:
    mov $77, %eax
    jmp .LRealJump

    .weak   pthread_mutexattr_setrobust_np
    .type   pthread_mutexattr_setrobust_np, @function
pthread_mutexattr_setrobust_np:
    mov $78, %eax
    jmp .LRealJump

    .weak   pthread_mutexattr_settype
    .type   pthread_mutexattr_settype, @function
pthread_mutexattr_settype:
    mov $79, %eax
    jmp .LRealJump


    .weak   pthread_once
    .type   pthread_once, @function
pthread_once:
    mov $80, %eax
    jmp .LRealJump

    .weak   pthread_rwlock_clockrdlock
    .type   pthread_rwlock_clockrdlock, @function
pthread_rwlock_clockrdlock:
    mov $81, %eax
    jmp .LRealJump

    .weak   pthread_rwlock_clockwrlock
    .type   pthread_rwlock_clockwrlock, @function
pthread_rwlock_clockwrlock:
    mov $82, %eax
    jmp .LRealJump

    .weak   pthread_rwlock_destroy
    .type   pthread_rwlock_destroy, @function
pthread_rwlock_destroy:
    mov $83, %eax
    jmp .LRealJump

    .weak   pthread_rwlock_init
    .type   pthread_rwlock_init, @function
pthread_rwlock_init:
    mov $84, %eax
    jmp .LRealJump

    .weak   pthread_rwlock_rdlock
    .type   pthread_rwlock_rdlock, @function
pthread_rwlock_rdlock:
    mov $85, %eax
    jmp .LRealJump

    .weak   pthread_rwlock_timedrdlock
    .type   pthread_rwlock_timedrdlock, @function
pthread_rwlock_timedrdlock:
    mov $86, %eax
    jmp .LRealJump

    .weak   pthread_rwlock_timedwrlock
    .type   pthread_rwlock_timedwrlock, @function
pthread_rwlock_timedwrlock:
    mov $87, %eax
    jmp .LRealJump

    .weak   pthread_rwlock_tryrdlock
    .type   pthread_rwlock_tryrdlock, @function
pthread_rwlock_tryrdlock:
    mov $88, %eax
    jmp .LRealJump

    .weak   pthread_rwlock_trywrlock
    .type   pthread_rwlock_trywrlock, @function
pthread_rwlock_trywrlock:
    mov $89, %eax
    jmp .LRealJump


    .weak   pthread_rwlock_unlock
    .type   pthread_rwlock_unlock, @function
pthread_rwlock_unlock:
    mov $90, %eax
    jmp .LRealJump

    .weak   pthread_rwlock_wrlock
    .type   pthread_rwlock_wrlock, @function
pthread_rwlock_wrlock:
    mov $91, %eax
    jmp .LRealJump

    .weak   pthread_rwlockattr_destroy
    .type   pthread_rwlockattr_destroy, @function
pthread_rwlockattr_destroy:
    mov $92, %eax
    jmp .LRealJump

    .weak   pthread_rwlockattr_getkind_np
    .type   pthread_rwlockattr_getkind_np, @function
pthread_rwlockattr_getkind_np:
    mov $93, %eax
    jmp .LRealJump

    .weak   pthread_rwlockattr_getpshared
    .type   pthread_rwlockattr_getpshared, @function
pthread_rwlockattr_getpshared:
    mov $94, %eax
    jmp .LRealJump

    .weak   pthread_rwlockattr_init
    .type   pthread_rwlockattr_init, @function
pthread_rwlockattr_init:
    mov $95, %eax
    jmp .LRealJump

    .weak   pthread_rwlockattr_setkind_np
    .type   pthread_rwlockattr_setkind_np, @function
pthread_rwlockattr_setkind_np:
    mov $96, %eax
    jmp .LRealJump

    .weak   pthread_rwlockattr_setpshared
    .type   pthread_rwlockattr_setpshared, @function
pthread_rwlockattr_setpshared:
    mov $97, %eax
    jmp .LRealJump

    .weak   pthread_setaffinity_np
    .type   pthread_setaffinity_np, @function
pthread_setaffinity_np:
    mov $98, %eax
    jmp .LRealJump

    .weak   pthread_setattr_default_np
    .type   pthread_setattr_default_np, @function
pthread_setattr_default_np:
    mov $99, %eax
    jmp .LRealJump


    .weak   pthread_setcancelstate
    .type   pthread_setcancelstate, @function
pthread_setcancelstate:
    mov $100, %eax
    jmp .LRealJump

    .weak   pthread_setcanceltype
    .type   pthread_setcanceltype, @function
pthread_setcanceltype:
    mov $101, %eax
    jmp .LRealJump

    .weak   pthread_setconcurrency
    .type   pthread_setconcurrency, @function
pthread_setconcurrency:
    mov $102, %eax
    jmp .LRealJump

    .weak   pthread_setname_np
    .type   pthread_setname_np, @function
pthread_setname_np:
    mov $103, %eax
    jmp .LRealJump

    .weak   pthread_setschedparam
    .type   pthread_setschedparam, @function
pthread_setschedparam:
    mov $104, %eax
    jmp .LRealJump

    .weak   pthread_setschedprio
    .type   pthread_setschedprio, @function
pthread_setschedprio:
    mov $105, %eax
    jmp .LRealJump

    .weak   pthread_setspecific
    .type   pthread_setspecific, @function
pthread_setspecific:
    mov $106, %eax
    jmp .LRealJump

    .weak   pthread_sigmask
    .type   pthread_sigmask, @function
pthread_sigmask:
    mov $107, %eax
    jmp .LRealJump

    .weak   pthread_sigqueue
    .type   pthread_sigqueue, @function
pthread_sigqueue:
    mov $108, %eax
    jmp .LRealJump

    .weak   pthread_spin_destroy
    .type   pthread_spin_destroy, @function
pthread_spin_destroy:
    mov $109, %eax
    jmp .LRealJump


    .weak   pthread_spin_init
    .type   pthread_spin_init, @function
pthread_spin_init:
    mov $110, %eax
    jmp .LRealJump

    .weak   pthread_spin_lock
    .type   pthread_spin_lock, @function
pthread_spin_lock:
    mov $111, %eax
    jmp .LRealJump

    .weak   pthread_spin_trylock
    .type   pthread_spin_trylock, @function
pthread_spin_trylock:
    mov $112, %eax
    jmp .LRealJump

    .weak   pthread_spin_unlock
    .type   pthread_spin_unlock, @function
pthread_spin_unlock:
    mov $113, %eax
    jmp .LRealJump

    .weak   pthread_testcancel
    .type   pthread_testcancel, @function
pthread_testcancel:
    mov $114, %eax
    jmp .LRealJump

    .weak   pthread_timedjoin_np
    .type   pthread_timedjoin_np, @function
pthread_timedjoin_np:
    mov $115, %eax
    jmp .LRealJump

    .weak   pthread_tryjoin_np
    .type   pthread_tryjoin_np, @function
pthread_tryjoin_np:
    mov $116, %eax
    jmp .LRealJump

    .weak   pthread_yield
    .type   pthread_yield, @function
pthread_yield:
    mov $117, %eax
    jmp .LRealJump    

.LRealJump:
    leaq    g_pthread_func_table(%rip), %r11
    mov (%r11, %rax, 8), %r11
    test %r11, %r11
    jz .LNeedInit
    jmp *%r11

.LNeedInit:
    
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rdi
    push %rsi
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    call InitLoadPthread
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rsi
    pop %rdi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    leaq    g_pthread_func_table(%rip), %r11
    mov (%r11, %rax, 8), %r11
    jmp *%r11
    
)");
#endif