/*
 * Copyright (c) 2005 The Regents of The University of Michigan
 * Copyright (c) 2007 MIPS Technologies, Inc.
 * Copyright (c) 2016 The University of Virginia
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Gabe Black
 *          Korey Sewell
 *          Alec Roelke
 */

#include "arch/ppu/linux/process.hh"

#include <map>

#include "arch/ppu/isa_traits.hh"
#include "arch/ppu/linux/linux.hh"
#include "base/loader/object_file.hh"
#include "base/trace.hh"
#include "ppu/thread_context.hh"
#include "debug/SyscallVerbose.hh"
#include "kern/linux/linux.hh"
#include "sim/eventq.hh"
#include "sim/process.hh"
#include "sim/syscall_desc.hh"
#include "sim/syscall_emul.hh"
#include "ppu_sim/system.hh"

using namespace std;
using namespace PpuISA;

namespace
{

class PpuLinuxObjectFileLoader : public PpuSOCProcess::Loader
{
  public:
    PpuSOCProcess *
    load(PpuSOCProcessParams *params, ObjectFile *obj_file) override
    {
        auto arch = obj_file->getArch();
        auto opsys = obj_file->getOpSys();

        if (arch != ObjectFile::Ppu64 && arch != ObjectFile::Ppu32)
            return nullptr;

        if (opsys == ObjectFile::UnknownOpSys) {
            warn("Unknown operating system; assuming Linux.");
            opsys = ObjectFile::Linux;
        }

        if (opsys != ObjectFile::Linux)
            return nullptr;

        if (arch == ObjectFile::Ppu64)
            return new PpuLinuxProcess64(params, obj_file);
        else
            return new PpuLinuxProcess32(params, obj_file);
    }
};

PpuLinuxObjectFileLoader loader;

} // anonymous namespace

/// Target uname() handler.
static SyscallReturn
unameFunc64(SyscallDesc *desc, int callnum, ThreadContext *tc_)
{
    int index = 0;
    PpuThreadContext *tc = dynamic_cast<PpuThreadContext*>(tc_);
    auto process = tc->PpugetProcessPtr();
    TypedBufferArg<Linux::utsname> name(process->getSyscallArg(tc, index));

    strcpy(name->sysname, "Linux");
    strcpy(name->nodename,"sim.gem5.org");
    strcpy(name->release, process->release.c_str());
    strcpy(name->version, "#1 Mon Aug 18 11:32:15 EDT 2003");
    strcpy(name->machine, "ppu64");

    name.copyOut(tc->getVirtProxy());
    return 0;
}

/// Target uname() handler.
static SyscallReturn
unameFunc32(SyscallDesc *desc, int callnum, ThreadContext *tc)
{
    int index = 0;
    PpuThreadContext *tc = dynamic_cast<PpuThreadContext*>(tc_);
    auto process = tc->PpugetProcessPtr();
    TypedBufferArg<Linux::utsname> name(process->getSyscallArg(tc, index));

    strcpy(name->sysname, "Linux");
    strcpy(name->nodename,"sim.gem5.org");
    strcpy(name->release, process->release.c_str());
    strcpy(name->version, "#1 Mon Aug 18 11:32:15 EDT 2003");
    strcpy(name->machine, "ppu32");

    name.copyOut(tc->getVirtProxy());
    return 0;
}

std::map<int, SyscallDescABI<DefaultSyscallABI>>
        PpuLinuxProcess64::syscallDescs = {
    {0,    { "io_setup" }},
    {1,    { "io_destroy" }},
    {2,    { "io_submit" }},
    {3,    { "io_cancel" }},
    {4,    { "io_getevents" }},
    {5,    { "setxattr" }},
    {6,    { "lsetxattr" }},
    {7,    { "fsetxattr" }},
    {8,    { "getxattr" }},
    {9,    { "lgetxattr" }},
    {10,   { "fgetxattr" }},
    {11,   { "listxattr" }},
    {12,   { "llistxattr" }},
    {13,   { "flistxattr" }},
    {14,   { "removexattr" }},
    {15,   { "lremovexattr" }},
    {16,   { "fremovexattr" }},
    {17,   { "getcwd", getcwdFunc }},
    {18,   { "lookup_dcookie" }},
    {19,   { "eventfd2" }},
    {20,   { "epoll_create1" }},
    {21,   { "epoll_ctl" }},
    {22,   { "epoll_pwait" }},
    {23,   { "dup", dupFunc }},
    {24,   { "dup3" }},
    {25,   { "fcntl", fcntl64Func }},
    {26,   { "inotify_init1" }},
    {27,   { "inotify_add_watch" }},
    {28,   { "inotify_rm_watch" }},
    {29,   { "ioctl", ioctlFunc<PpuLinux64> }},
    {30,   { "ioprio_get" }},
    {31,   { "ioprio_set" }},
    {32,   { "flock" }},
    {33,   { "mknodat" }},
    {34,   { "mkdirat" }},
    {35,   { "unlinkat", unlinkatFunc<PpuLinux64> }},
    {36,   { "symlinkat" }},
    {37,   { "linkat" }},
    {38,   { "renameat", renameatFunc<PpuLinux64> }},
    {39,   { "umount2" }},
    {40,   { "mount" }},
    {41,   { "pivot_root" }},
    {42,   { "nfsservctl" }},
    {43,   { "statfs", statfsFunc<PpuLinux64> }},
    {44,   { "fstatfs", fstatfsFunc<PpuLinux64> }},
    {45,   { "truncate", truncateFunc }},
    {46,   { "ftruncate", ftruncate64Func }},
    {47,   { "fallocate", fallocateFunc }},
    {48,   { "faccessat", faccessatFunc<PpuLinux64> }},
    {49,   { "chdir" }},
    {50,   { "fchdir" }},
    {51,   { "chroot" }},
    {52,   { "fchmod", fchmodFunc<PpuLinux64> }},
    {53,   { "fchmodat" }},
    {54,   { "fchownat" }},
    {55,   { "fchown", fchownFunc }},
    {56,   { "openat", openatFunc<PpuLinux64> }},
    {57,   { "close", closeFunc }},
    {58,   { "vhangup" }},
    {59,   { "pipe2" }},
    {60,   { "quotactl" }},
    {61,   { "getdents64" }},
    {62,   { "lseek", lseekFunc }},
    {63,   { "read", readFunc<PpuLinux64> }},
    {64,   { "write", writeFunc<PpuLinux64> }},
    {66,   { "writev", writevFunc<PpuLinux64> }},
    {67,   { "pread64" }},
    {68,   { "pwrite64", pwrite64Func<PpuLinux64> }},
    {69,   { "preadv" }},
    {70,   { "pwritev" }},
    {71,   { "sendfile" }},
    {72,   { "pselect6" }},
    {73,   { "ppoll" }},
    {74,   { "signalfd64" }},
    {75,   { "vmsplice" }},
    {76,   { "splice" }},
    {77,   { "tee" }},
    {78,   { "readlinkat", readlinkatFunc<PpuLinux64> }},
    {79,   { "fstatat", fstatat64Func<PpuLinux64> }},
    {80,   { "fstat", fstat64Func<PpuLinux64> }},
    {81,   { "sync" }},
    {82,   { "fsync" }},
    {83,   { "fdatasync" }},
    {84,   { "sync_file_range2" }},
    {85,   { "timerfd_create" }},
    {86,   { "timerfd_settime" }},
    {87,   { "timerfd_gettime" }},
    {88,   { "utimensat" }},
    {89,   { "acct" }},
    {90,   { "capget" }},
    {91,   { "capset" }},
    {92,   { "personality" }},
    {93,   { "exit", exitFunc }},
    {94,   { "exit_group", exitGroupFunc }},
    {95,   { "waitid" }},
    {96,   { "set_tid_address", setTidAddressFunc }},
    {97,   { "unshare" }},
    {98,   { "futex", futexFunc<PpuLinux64> }},
    {99,   { "set_robust_list", ignoreWarnOnceFunc }},
    {100,  { "get_robust_list", ignoreWarnOnceFunc }},
    {101,  { "nanosleep", ignoreWarnOnceFunc }},
    {102,  { "getitimer" }},
    {103,  { "setitimer" }},
    {104,  { "kexec_load" }},
    {105,  { "init_module" }},
    {106,  { "delete_module" }},
    {107,  { "timer_create" }},
    {108,  { "timer_gettime" }},
    {109,  { "timer_getoverrun" }},
    {110,  { "timer_settime" }},
    {111,  { "timer_delete" }},
    {112,  { "clock_settime" }},
    {113,  { "clock_gettime", clock_gettimeFunc<PpuLinux64> }},
    {114,  { "clock_getres", clock_getresFunc<PpuLinux64> }},
    {115,  { "clock_nanosleep" }},
    {116,  { "syslog" }},
    {117,  { "ptrace" }},
    {118,  { "sched_setparam" }},
    {119,  { "sched_setscheduler" }},
    {120,  { "sched_getscheduler" }},
    {121,  { "sched_getparam" }},
    {122,  { "sched_setaffinity" }},
    {123,  { "sched_getaffinity" }},
    {124,  { "sched_yield", ignoreWarnOnceFunc }},
    {125,  { "sched_get_priority_max" }},
    {126,  { "sched_get_priority_min" }},
    {127,  { "scheD_rr_get_interval" }},
    {128,  { "restart_syscall" }},
    {129,  { "kill" }},
    {130,  { "tkill" }},
    {131,  { "tgkill", tgkillFunc<PpuLinux64> }},
    {132,  { "sigaltstack" }},
    {133,  { "rt_sigsuspend", ignoreWarnOnceFunc }},
    {134,  { "rt_sigaction", ignoreWarnOnceFunc }},
    {135,  { "rt_sigprocmask", ignoreWarnOnceFunc }},
    {136,  { "rt_sigpending", ignoreWarnOnceFunc }},
    {137,  { "rt_sigtimedwait", ignoreWarnOnceFunc }},
    {138,  { "rt_sigqueueinfo", ignoreWarnOnceFunc }},
    {139,  { "rt_sigreturn", ignoreWarnOnceFunc }},
    {140,  { "setpriority" }},
    {141,  { "getpriority" }},
    {142,  { "reboot" }},
    {143,  { "setregid" }},
    {144,  { "setgid" }},
    {145,  { "setreuid" }},
    {146,  { "setuid", ignoreFunc }},
    {147,  { "setresuid" }},
    {148,  { "getresuid" }},
    {149,  { "getresgid" }},
    {150,  { "getresgid" }},
    {151,  { "setfsuid" }},
    {152,  { "setfsgid" }},
    {153,  { "times", timesFunc<PpuLinux64> }},
    {154,  { "setpgid", setpgidFunc }},
    {155,  { "getpgid" }},
    {156,  { "getsid" }},
    {157,  { "setsid" }},
    {158,  { "getgroups" }},
    {159,  { "setgroups" }},
    {160,  { "uname", unameFunc64 }},
    {161,  { "sethostname" }},
    {162,  { "setdomainname" }},
    {163,  { "getrlimit", getrlimitFunc<PpuLinux64> }},
    {164,  { "setrlimit", ignoreFunc }},
    {165,  { "getrusage", getrusageFunc<PpuLinux64> }},
    {166,  { "umask", umaskFunc }},
    {167,  { "prctl" }},
    {168,  { "getcpu" }},
    {169,  { "gettimeofday", gettimeofdayFunc<PpuLinux64> }},
    {170,  { "settimeofday" }},
    {171,  { "adjtimex" }},
    {172,  { "getpid", getpidFunc }},
    {173,  { "getppid", getppidFunc }},
    {174,  { "getuid", getuidFunc }},
    {175,  { "geteuid", geteuidFunc }},
    {176,  { "getgid", getgidFunc }},
    {177,  { "getegid", getegidFunc }},
    {178,  { "gettid", gettidFunc }},
    {179,  { "sysinfo", sysinfoFunc<PpuLinux64> }},
    {180,  { "mq_open" }},
    {181,  { "mq_unlink" }},
    {182,  { "mq_timedsend" }},
    {183,  { "mq_timedrecieve" }},
    {184,  { "mq_notify" }},
    {185,  { "mq_getsetattr" }},
    {186,  { "msgget" }},
    {187,  { "msgctl" }},
    {188,  { "msgrcv" }},
    {189,  { "msgsnd" }},
    {190,  { "semget" }},
    {191,  { "semctl" }},
    {192,  { "semtimedop" }},
    {193,  { "semop" }},
    {194,  { "shmget" }},
    {195,  { "shmctl" }},
    {196,  { "shmat" }},
    {197,  { "shmdt" }},
    {198,  { "socket" }},
    {199,  { "socketpair" }},
    {200,  { "bind" }},
    {201,  { "listen" }},
    {202,  { "accept" }},
    {203,  { "connect" }},
    {204,  { "getsockname" }},
    {205,  { "getpeername" }},
    {206,  { "sendo" }},
    {207,  { "recvfrom" }},
    {208,  { "setsockopt" }},
    {209,  { "getsockopt" }},
    {210,  { "shutdown" }},
    {211,  { "sendmsg" }},
    {212,  { "recvmsg" }},
    {213,  { "readahead" }},
    {214,  { "brk", brkFunc }},
    {215,  { "munmap", munmapFunc }},
    {216,  { "mremap", mremapFunc<PpuLinux64> }},
    {217,  { "add_key" }},
    {218,  { "request_key" }},
    {219,  { "keyctl" }},
    {220,  { "clone", cloneFunc<PpuLinux64> }},
    {221,  { "execve", execveFunc<PpuLinux64> }},
    {222,  { "mmap", mmapFunc<PpuLinux64> }},
    {223,  { "fadvise64" }},
    {224,  { "swapon" }},
    {225,  { "swapoff" }},
    {226,  { "mprotect", ignoreFunc }},
    {227,  { "msync", ignoreFunc }},
    {228,  { "mlock", ignoreFunc }},
    {229,  { "munlock", ignoreFunc }},
    {230,  { "mlockall", ignoreFunc }},
    {231,  { "munlockall", ignoreFunc }},
    {232,  { "mincore", ignoreFunc }},
    {233,  { "madvise", ignoreFunc }},
    {234,  { "remap_file_pages" }},
    {235,  { "mbind", ignoreFunc }},
    {236,  { "get_mempolicy" }},
    {237,  { "set_mempolicy" }},
    {238,  { "migrate_pages" }},
    {239,  { "move_pages" }},
    {240,  { "tgsigqueueinfo" }},
    {241,  { "perf_event_open" }},
    {242,  { "accept4" }},
    {243,  { "recvmmsg" }},
    {260,  { "wait4" }},
    {261,  { "prlimit64", prlimitFunc<PpuLinux64> }},
    {262,  { "fanotify_init" }},
    {263,  { "fanotify_mark" }},
    {264,  { "name_to_handle_at" }},
    {265,  { "open_by_handle_at" }},
    {266,  { "clock_adjtime" }},
    {267,  { "syncfs" }},
    {268,  { "setns" }},
    {269,  { "sendmmsg" }},
    {270,  { "process_vm_ready" }},
    {271,  { "process_vm_writev" }},
    {272,  { "kcmp" }},
    {273,  { "finit_module" }},
    {274,  { "sched_setattr" }},
    {275,  { "sched_getattr" }},
    {276,  { "renameat2" }},
    {277,  { "seccomp" }},
    {278,  { "getrandom" }},
    {279,  { "memfd_create" }},
    {280,  { "bpf" }},
    {281,  { "execveat" }},
    {282,  { "userfaultid" }},
    {283,  { "membarrier" }},
    {284,  { "mlock2" }},
    {285,  { "copy_file_range" }},
    {286,  { "preadv2" }},
    {287,  { "pwritev2" }},
    {1024, { "open", openFunc<PpuLinux64> }},
    {1025, { "link" }},
    {1026, { "unlink", unlinkFunc }},
    {1027, { "mknod" }},
    {1028, { "chmod", chmodFunc<PpuLinux64> }},
    {1029, { "chown", chownFunc }},
    {1030, { "mkdir", mkdirFunc }},
    {1031, { "rmdir" }},
    {1032, { "lchown" }},
    {1033, { "access", accessFunc }},
    {1034, { "rename", renameFunc }},
    {1035, { "readlink", readlinkFunc }},
    {1036, { "symlink" }},
    {1037, { "utimes", utimesFunc<PpuLinux64> }},
    {1038, { "stat", stat64Func<PpuLinux64> }},
    {1039, { "lstat", lstat64Func<PpuLinux64> }},
    {1040, { "pipe", pipeFunc }},
    {1041, { "dup2", dup2Func }},
    {1042, { "epoll_create" }},
    {1043, { "inotifiy_init" }},
    {1044, { "eventfd" }},
    {1045, { "signalfd" }},
    {1046, { "sendfile" }},
    {1047, { "ftruncate", ftruncate64Func }},
    {1048, { "truncate", truncate64Func }},
    {1049, { "stat", stat64Func<PpuLinux64> }},
    {1050, { "lstat", lstat64Func<PpuLinux64> }},
    {1051, { "fstat", fstat64Func<PpuLinux64> }},
    {1052, { "fcntl", fcntl64Func }},
    {1053, { "fadvise64" }},
    {1054, { "newfstatat" }},
    {1055, { "fstatfs", fstatfsFunc<PpuLinux64> }},
    {1056, { "statfs", statfsFunc<PpuLinux64> }},
    {1057, { "lseek", lseekFunc }},
    {1058, { "mmap", mmapFunc<PpuLinux64> }},
    {1059, { "alarm" }},
    {1060, { "getpgrp" }},
    {1061, { "pause" }},
    {1062, { "time", timeFunc<PpuLinux64> }},
    {1063, { "utime" }},
    {1064, { "creat" }},
    {1065, { "getdents" }},
    {1066, { "futimesat" }},
    {1067, { "select" }},
    {1068, { "poll" }},
    {1069, { "epoll_wait" }},
    {1070, { "ustat" }},
    {1071, { "vfork" }},
    {1072, { "oldwait4" }},
    {1073, { "recv" }},
    {1074, { "send" }},
    {1075, { "bdflush" }},
    {1076, { "umount" }},
    {1077, { "uselib" }},
    {1078, { "sysctl" }},
    {1079, { "fork" }},
    {2011, { "getmainvars" }}
};

std::map<int, SyscallDescABI<DefaultSyscallABI>>
        PpuLinuxProcess32::syscallDescs = {
    {0,    { "io_setup" }},
    {1,    { "io_destroy" }},
    {2,    { "io_submit" }},
    {3,    { "io_cancel" }},
    {4,    { "io_getevents" }},
    {5,    { "setxattr" }},
    {6,    { "lsetxattr" }},
    {7,    { "fsetxattr" }},
    {8,    { "getxattr" }},
    {9,    { "lgetxattr" }},
    {10,   { "fgetxattr" }},
    {11,   { "listxattr" }},
    {12,   { "llistxattr" }},
    {13,   { "flistxattr" }},
    {14,   { "removexattr" }},
    {15,   { "lremovexattr" }},
    {16,   { "fremovexattr" }},
    {17,   { "getcwd", getcwdFunc }},
    {18,   { "lookup_dcookie" }},
    {19,   { "eventfd2" }},
    {20,   { "epoll_create1" }},
    {21,   { "epoll_ctl" }},
    {22,   { "epoll_pwait" }},
    {23,   { "dup", dupFunc }},
    {24,   { "dup3" }},
    {25,   { "fcntl", fcntlFunc }},
    {26,   { "inotify_init1" }},
    {27,   { "inotify_add_watch" }},
    {28,   { "inotify_rm_watch" }},
    {29,   { "ioctl", ioctlFunc<PpuLinux32> }},
    {30,   { "ioprio_get" }},
    {31,   { "ioprio_set" }},
    {32,   { "flock" }},
    {33,   { "mknodat" }},
    {34,   { "mkdirat" }},
    {35,   { "unlinkat", unlinkatFunc<PpuLinux32> }},
    {36,   { "symlinkat" }},
    {37,   { "linkat" }},
    {38,   { "renameat", renameatFunc<PpuLinux32> }},
    {39,   { "umount2" }},
    {40,   { "mount" }},
    {41,   { "pivot_root" }},
    {42,   { "nfsservctl" }},
    {43,   { "statfs", statfsFunc<PpuLinux32> }},
    {44,   { "fstatfs", fstatfsFunc<PpuLinux32> }},
    {45,   { "truncate", truncateFunc }},
    {46,   { "ftruncate", ftruncateFunc }},
    {47,   { "fallocate", fallocateFunc }},
    {48,   { "faccessat", faccessatFunc<PpuLinux32> }},
    {49,   { "chdir" }},
    {50,   { "fchdir" }},
    {51,   { "chroot" }},
    {52,   { "fchmod", fchmodFunc<PpuLinux32> }},
    {53,   { "fchmodat" }},
    {54,   { "fchownat" }},
    {55,   { "fchown", fchownFunc }},
    {56,   { "openat", openatFunc<PpuLinux32> }},
    {57,   { "close", closeFunc }},
    {58,   { "vhangup" }},
    {59,   { "pipe2" }},
    {60,   { "quotactl" }},
    {61,   { "getdents64" }},
    {62,   { "lseek", lseekFunc }},
    {63,   { "read", readFunc<PpuLinux32> }},
    {64,   { "write", writeFunc<PpuLinux32> }},
    {66,   { "writev", writevFunc<PpuLinux32> }},
    {67,   { "pread64" }},
    {68,   { "pwrite64", pwrite64Func<PpuLinux32> }},
    {69,   { "preadv" }},
    {70,   { "pwritev" }},
    {71,   { "sendfile" }},
    {72,   { "pselect6" }},
    {73,   { "ppoll" }},
    {74,   { "signalfd64" }},
    {75,   { "vmsplice" }},
    {76,   { "splice" }},
    {77,   { "tee" }},
    {78,   { "readlinkat", readlinkatFunc<PpuLinux32> }},
    {79,   { "fstatat" }},
    {80,   { "fstat", fstatFunc<PpuLinux32> }},
    {81,   { "sync" }},
    {82,   { "fsync" }},
    {83,   { "fdatasync" }},
    {84,   { "sync_file_range2" }},
    {85,   { "timerfd_create" }},
    {86,   { "timerfd_settime" }},
    {87,   { "timerfd_gettime" }},
    {88,   { "utimensat" }},
    {89,   { "acct" }},
    {90,   { "capget" }},
    {91,   { "capset" }},
    {92,   { "personality" }},
    {93,   { "exit", exitFunc }},
    {94,   { "exit_group", exitGroupFunc }},
    {95,   { "waitid" }},
    {96,   { "set_tid_address", setTidAddressFunc }},
    {97,   { "unshare" }},
    {98,   { "futex", futexFunc<PpuLinux32> }},
    {99,   { "set_robust_list", ignoreWarnOnceFunc }},
    {100,  { "get_robust_list", ignoreWarnOnceFunc }},
    {101,  { "nanosleep" }},
    {102,  { "getitimer" }},
    {103,  { "setitimer" }},
    {104,  { "kexec_load" }},
    {105,  { "init_module" }},
    {106,  { "delete_module" }},
    {107,  { "timer_create" }},
    {108,  { "timer_gettime" }},
    {109,  { "timer_getoverrun" }},
    {110,  { "timer_settime" }},
    {111,  { "timer_delete" }},
    {112,  { "clock_settime" }},
    {113,  { "clock_gettime", clock_gettimeFunc<PpuLinux32> }},
    {114,  { "clock_getres", clock_getresFunc<PpuLinux32> }},
    {115,  { "clock_nanosleep" }},
    {116,  { "syslog" }},
    {117,  { "ptrace" }},
    {118,  { "sched_setparam" }},
    {119,  { "sched_setscheduler" }},
    {120,  { "sched_getscheduler" }},
    {121,  { "sched_getparam" }},
    {122,  { "sched_setaffinity" }},
    {123,  { "sched_getaffinity" }},
    {124,  { "sched_yield", ignoreWarnOnceFunc }},
    {125,  { "sched_get_priority_max" }},
    {126,  { "sched_get_priority_min" }},
    {127,  { "scheD_rr_get_interval" }},
    {128,  { "restart_syscall" }},
    {129,  { "kill" }},
    {130,  { "tkill" }},
    {131,  { "tgkill", tgkillFunc<PpuLinux32> }},
    {132,  { "sigaltstack" }},
    {133,  { "rt_sigsuspend", ignoreWarnOnceFunc }},
    {134,  { "rt_sigaction", ignoreWarnOnceFunc }},
    {135,  { "rt_sigprocmask", ignoreWarnOnceFunc }},
    {136,  { "rt_sigpending", ignoreWarnOnceFunc }},
    {137,  { "rt_sigtimedwait", ignoreWarnOnceFunc }},
    {138,  { "rt_sigqueueinfo", ignoreWarnOnceFunc }},
    {139,  { "rt_sigreturn", ignoreWarnOnceFunc }},
    {140,  { "setpriority" }},
    {141,  { "getpriority" }},
    {142,  { "reboot" }},
    {143,  { "setregid" }},
    {144,  { "setgid" }},
    {145,  { "setreuid" }},
    {146,  { "setuid", ignoreFunc }},
    {147,  { "setresuid" }},
    {148,  { "getresuid" }},
    {149,  { "getresgid" }},
    {150,  { "getresgid" }},
    {151,  { "setfsuid" }},
    {152,  { "setfsgid" }},
    {153,  { "times", timesFunc<PpuLinux32> }},
    {154,  { "setpgid", setpgidFunc }},
    {155,  { "getpgid" }},
    {156,  { "getsid" }},
    {157,  { "setsid" }},
    {158,  { "getgroups" }},
    {159,  { "setgroups" }},
    {160,  { "uname", unameFunc32 }},
    {161,  { "sethostname" }},
    {162,  { "setdomainname" }},
    {163,  { "getrlimit", getrlimitFunc<PpuLinux32> }},
    {164,  { "setrlimit", ignoreFunc }},
    {165,  { "getrusage", getrusageFunc<PpuLinux32> }},
    {166,  { "umask", umaskFunc }},
    {167,  { "prctl" }},
    {168,  { "getcpu" }},
    {169,  { "gettimeofday", gettimeofdayFunc<PpuLinux32> }},
    {170,  { "settimeofday" }},
    {171,  { "adjtimex" }},
    {172,  { "getpid", getpidFunc }},
    {173,  { "getppid", getppidFunc }},
    {174,  { "getuid", getuidFunc }},
    {175,  { "geteuid", geteuidFunc }},
    {176,  { "getgid", getgidFunc }},
    {177,  { "getegid", getegidFunc }},
    {178,  { "gettid", gettidFunc }},
    {179,  { "sysinfo", sysinfoFunc<PpuLinux32> }},
    {180,  { "mq_open" }},
    {181,  { "mq_unlink" }},
    {182,  { "mq_timedsend" }},
    {183,  { "mq_timedrecieve" }},
    {184,  { "mq_notify" }},
    {185,  { "mq_getsetattr" }},
    {186,  { "msgget" }},
    {187,  { "msgctl" }},
    {188,  { "msgrcv" }},
    {189,  { "msgsnd" }},
    {190,  { "semget" }},
    {191,  { "semctl" }},
    {192,  { "semtimedop" }},
    {193,  { "semop" }},
    {194,  { "shmget" }},
    {195,  { "shmctl" }},
    {196,  { "shmat" }},
    {197,  { "shmdt" }},
    {198,  { "socket" }},
    {199,  { "socketpair" }},
    {200,  { "bind" }},
    {201,  { "listen" }},
    {202,  { "accept" }},
    {203,  { "connect" }},
    {204,  { "getsockname" }},
    {205,  { "getpeername" }},
    {206,  { "sendo" }},
    {207,  { "recvfrom" }},
    {208,  { "setsockopt" }},
    {209,  { "getsockopt" }},
    {210,  { "shutdown" }},
    {211,  { "sendmsg" }},
    {212,  { "recvmsg" }},
    {213,  { "readahead" }},
    {214,  { "brk", brkFunc }},
    {215,  { "munmap", munmapFunc }},
    {216,  { "mremap", mremapFunc<PpuLinux32> }},
    {217,  { "add_key" }},
    {218,  { "request_key" }},
    {219,  { "keyctl" }},
    {220,  { "clone", cloneFunc<PpuLinux32> }},
    {221,  { "execve", execveFunc<PpuLinux32> }},
    {222,  { "mmap", mmapFunc<PpuLinux32> }},
    {223,  { "fadvise64" }},
    {224,  { "swapon" }},
    {225,  { "swapoff" }},
    {226,  { "mprotect", ignoreFunc }},
    {227,  { "msync", ignoreFunc }},
    {228,  { "mlock", ignoreFunc }},
    {229,  { "munlock", ignoreFunc }},
    {230,  { "mlockall", ignoreFunc }},
    {231,  { "munlockall", ignoreFunc }},
    {232,  { "mincore", ignoreFunc }},
    {233,  { "madvise", ignoreFunc }},
    {234,  { "remap_file_pages" }},
    {235,  { "mbind", ignoreFunc }},
    {236,  { "get_mempolicy" }},
    {237,  { "set_mempolicy" }},
    {238,  { "migrate_pages" }},
    {239,  { "move_pages" }},
    {240,  { "tgsigqueueinfo" }},
    {241,  { "perf_event_open" }},
    {242,  { "accept4" }},
    {243,  { "recvmmsg" }},
    {260,  { "wait4" }},
    {261,  { "prlimit64", prlimitFunc<PpuLinux32> }},
    {262,  { "fanotify_init" }},
    {263,  { "fanotify_mark" }},
    {264,  { "name_to_handle_at" }},
    {265,  { "open_by_handle_at" }},
    {266,  { "clock_adjtime" }},
    {267,  { "syncfs" }},
    {268,  { "setns" }},
    {269,  { "sendmmsg" }},
    {270,  { "process_vm_ready" }},
    {271,  { "process_vm_writev" }},
    {272,  { "kcmp" }},
    {273,  { "finit_module" }},
    {274,  { "sched_setattr" }},
    {275,  { "sched_getattr" }},
    {276,  { "renameat2" }},
    {277,  { "seccomp" }},
    {278,  { "getrandom" }},
    {279,  { "memfd_create" }},
    {280,  { "bpf" }},
    {281,  { "execveat" }},
    {282,  { "userfaultid" }},
    {283,  { "membarrier" }},
    {284,  { "mlock2" }},
    {285,  { "copy_file_range" }},
    {286,  { "preadv2" }},
    {287,  { "pwritev2" }},
    {1024, { "open", openFunc<PpuLinux32> }},
    {1025, { "link" }},
    {1026, { "unlink", unlinkFunc }},
    {1027, { "mknod" }},
    {1028, { "chmod", chmodFunc<PpuLinux32> }},
    {1029, { "chown", chownFunc }},
    {1030, { "mkdir", mkdirFunc }},
    {1031, { "rmdir" }},
    {1032, { "lchown" }},
    {1033, { "access", accessFunc }},
    {1034, { "rename", renameFunc }},
    {1035, { "readlink", readlinkFunc }},
    {1036, { "symlink" }},
    {1037, { "utimes", utimesFunc<PpuLinux32> }},
    {1038, { "stat", statFunc<PpuLinux32> }},
    {1039, { "lstat", lstatFunc<PpuLinux32> }},
    {1040, { "pipe", pipeFunc }},
    {1041, { "dup2", dup2Func }},
    {1042, { "epoll_create" }},
    {1043, { "inotifiy_init" }},
    {1044, { "eventfd" }},
    {1045, { "signalfd" }},
    {1046, { "sendfile" }},
    {1047, { "ftruncate", ftruncateFunc }},
    {1048, { "truncate", truncateFunc }},
    {1049, { "stat", statFunc<PpuLinux32> }},
    {1050, { "lstat", lstatFunc<PpuLinux32> }},
    {1051, { "fstat", fstatFunc<PpuLinux32> }},
    {1052, { "fcntl", fcntlFunc }},
    {1053, { "fadvise64" }},
    {1054, { "newfstatat" }},
    {1055, { "fstatfs", fstatfsFunc<PpuLinux32> }},
    {1056, { "statfs", statfsFunc<PpuLinux32> }},
    {1057, { "lseek", lseekFunc }},
    {1058, { "mmap", mmapFunc<PpuLinux32> }},
    {1059, { "alarm" }},
    {1060, { "getpgrp" }},
    {1061, { "pause" }},
    {1062, { "time", timeFunc<PpuLinux32> }},
    {1063, { "utime" }},
    {1064, { "creat" }},
    {1065, { "getdents" }},
    {1066, { "futimesat" }},
    {1067, { "select" }},
    {1068, { "poll" }},
    {1069, { "epoll_wait" }},
    {1070, { "ustat" }},
    {1071, { "vfork" }},
    {1072, { "oldwait4" }},
    {1073, { "recv" }},
    {1074, { "send" }},
    {1075, { "bdflush" }},
    {1076, { "umount" }},
    {1077, { "uselib" }},
    {1078, { "sysctl" }},
    {1079, { "fork" }},
    {2011, { "getmainvars" }}
};

PpuLinuxProcess64::PpuLinuxProcess64(PpuSOCProcessParams * params,
    ObjectFile *objFile) : PpuProcess64(params, objFile)
{}

SyscallDesc*
PpuLinuxProcess64::getDesc(int callnum)
{
    return syscallDescs.find(callnum) != syscallDescs.end() ?
        &syscallDescs.at(callnum) : nullptr;
}

void
PpuLinuxProcess64::syscall(ThreadContext *tc, Fault *fault)
{
    doSyscall(tc->readIntReg(SyscallNumReg), tc, fault);
}

PpuLinuxProcess32::PpuLinuxProcess32(PpuSOCProcessParams * params,
    ObjectFile *objFile) : PpuProcess32(params, objFile)
{}

SyscallDesc*
PpuLinuxProcess32::getDesc(int callnum)
{
    return syscallDescs.find(callnum) != syscallDescs.end() ?
        &syscallDescs.at(callnum) : nullptr;
}

void
PpuLinuxProcess32::syscall(ThreadContext *tc, Fault *fault)
{
    doSyscall(tc->readIntReg(SyscallNumReg), tc, fault);
}
