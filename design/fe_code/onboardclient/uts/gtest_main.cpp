/**********************************************
 *********   UTS VERSION : 1.5.0-a1    *********
 **********************************************/



#ifdef inline
#undef inline
#endif
#include "gtest/gtest.h"
#include <stdio.h>
#ifdef __LINUX__
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

// To reference current execution file name
extern char * program_invocation_name;
int gdb_ret = -1;

std::string do_popen(std::string command){
    FILE *fp = NULL;
    std::string result = "";
    char line[1024];
    memset(line, '\0', sizeof(line)*sizeof(char));
    if((fp = popen(command.c_str(), "r")) == NULL)
        exit(1);

    while(fgets(line, 1024, fp) != NULL) {
        result.append(line);
    }

    gdb_ret = pclose(fp);

    return result;
}

// When SIGSEGV is occurred during gdb running,
// We need to do workaround for print backtrace.
void gdb_seg_handler(int sig) {
    exit(1);
}

void handler(int sig) {
    void *array[32];
    size_t backtrace_size;
    char ** frame_chars;
    std::string frame_string;
    std::string gdb_command, check_gdb_init_command;
    std::string console_output;

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    fprintf(stderr, "[31m==================      backtrace       =================[0m\n");

    gdb_command = "LD_LIBRARY_PATH='' gdb -q -batch -ex 'run' -ex 'backtrace' " + std::string(program_invocation_name);
    check_gdb_init_command = "[ -f $HOME/.gdbinit ] && echo \"Found .gdbinit file\" || echo \"set env LD_LIBRARY_PATH $LD_LIBRARY_PATH\" > $HOME/.gdbinit";

    // Disable Signal
    //signal(SIGSEGV, SIG_DFL);
    signal(SIGSEGV, gdb_seg_handler);

    console_output = do_popen(check_gdb_init_command);
    std::cout << console_output << std::endl;

    console_output = do_popen(gdb_command);
    std::cout << console_output << std::endl;

    // If can't use gdb, we use backtrace_symbols.
    if(gdb_ret != 0) {

        backtrace_size = backtrace(array, 32);
        frame_chars = backtrace_symbols(array, backtrace_size);

        for(unsigned int i=2; i<backtrace_size; i++)
        {
            std::string bin = "";
            std::string s_addr  = "";
            std::string func  = "";
            frame_string = std::string(frame_chars[i]);
            std::string addr2line_command = "addr2line --demangle -f -e ";
            size_t ms_pos = frame_string.find("(");
            size_t me_pos = frame_string.find(")");
            size_t s_pos = frame_string.find("[");
            size_t e_pos = frame_string.find("]");
            std::string s_line = "";
            if(ms_pos != std::string::npos)
                bin = frame_string.substr(0,ms_pos);

            if(ms_pos != me_pos-1 )
            {
                size_t p_pos = frame_string.find("+");
                if(p_pos != std::string::npos)
                    func = frame_string.substr(p_pos+1, me_pos-1 - p_pos );
                else
                    func = frame_string.substr(ms_pos+1, me_pos-1 - ms_pos );

                if(func.find("0x") == 0){
                    s_line = do_popen( addr2line_command + bin + " " + func);
                    if(s_line.size() == 0)
                        continue;
                    func = s_line;
                }
                std::string nm_command[] = { "nm ", "nm -D "};
                for(int i=0; i<2; i++){
                    nm_command[i].append(bin+ " | grep " + func);
                    s_line = do_popen( nm_command[i]);
                    if(s_line.size() != 0)
                        break;
                }

                if(s_line.find(" ") != std::string::npos)
                    s_addr =  "0x"+ s_line.substr(0, s_line.find(" "));

            }else if(s_pos != std::string::npos && e_pos != std::string::npos )
                s_addr = frame_string.substr(s_pos+1, e_pos-s_pos-1);

            if(bin != "" && s_addr != ""){
                addr2line_command.append(bin + " " + s_addr);
                s_line = do_popen( addr2line_command);
                if(s_line.size() != 0){
                    fprintf(stderr, "[[31m%2d[0m] %s",i-2, s_line.c_str());
                    if(s_line.find("main\n") == 0)
                        break;
                }
            }
        }

        if(frame_chars != NULL)
            free(frame_chars);
        exit(1);
    }

    fprintf(stderr, "[31m==============================================================[0m\n");
}
#elif defined(__ANDROID__)
#define UNWIND_BACKTRACE_WITH_SKIPPING_METHOD 1

#define HIDE_EXPORTS 1
#include <unwind.h>

#define ENABLE_DEMANGLING 1
#if __cplusplus
extern "C"
#endif
char* __cxa_demangle(
        const char* mangled_name,
        char* output_buffer,
        size_t* length,
        int* status);

#include <assert.h>
#include <dlfcn.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


static const size_t address_count_max = 30;

struct BacktraceState {
    // On ARM32 architecture this context is needed
    // for getting backtrace of the before-crash stack,
    // not of the signal handler stack.
    const ucontext_t*   signal_ucontext;

    // On non-ARM32 architectures signal handler stack
    // seems to be "connected" to the before-crash stack,
    // so we only need to skip several initial addresses.
    size_t              address_skip_count;

    size_t              address_count;
    uintptr_t           addresses[address_count_max];

};
typedef struct BacktraceState BacktraceState;


void BacktraceState_Init(BacktraceState* state, const ucontext_t* ucontext) {
    assert(state);
    assert(ucontext);
    memset(state, 0, sizeof(BacktraceState));
    state->signal_ucontext = ucontext;
    state->address_skip_count = 3;
}

bool BacktraceState_AddAddress(BacktraceState* state, uintptr_t ip) {
    assert(state);

    // No more space in the storage. Fail.
    if (state->address_count >= address_count_max)
        return false;

#if __thumb__
    // Reset the Thumb bit, if it is set.
    const uintptr_t thumb_bit = 1;
    ip &= ~thumb_bit;
#endif

    if (state->address_count > 0) {
        // Ignore null addresses.
        // They sometimes happen when using _Unwind_Backtrace()
        // with the compiler optimizations,
        // when the Link Register is overwritten by the inner
        // stack frames, like PreCrash() functions in this example.
        if (ip == 0)
            return true;

        // Ignore duplicate addresses.
        // They sometimes happen when using _Unwind_Backtrace()
        // with the compiler optimizations,
        // because we both add the second address from the Link Register
        // in ProcessRegisters() and receive the same address
        // in UnwindBacktraceCallback().
        if (ip == state->addresses[state->address_count - 1])
            return true;
    }

    // Finally add the address to the storage.
    state->addresses[state->address_count++] = ip;
    return true;
}


#if LIBUNWIND_WITH_REGISTERS_METHOD

void LibunwindWithRegisters(BacktraceState* state) {
    assert(state);

    // Initialize unw_context and unw_cursor.
    unw_context_t unw_context = {};
    unw_getcontext(&unw_context);
    unw_cursor_t  unw_cursor = {};
    unw_init_local(&unw_cursor, &unw_context);

    // Get more contexts.
    const ucontext_t* signal_ucontext = state->signal_ucontext;
    assert(signal_ucontext);
    const struct sigcontext* signal_mcontext = &(signal_ucontext->uc_mcontext);
    assert(signal_mcontext);

    // Set registers.
    unw_set_reg(&unw_cursor, UNW_ARM_R0,  signal_mcontext->arm_r0);
    unw_set_reg(&unw_cursor, UNW_ARM_R1,  signal_mcontext->arm_r1);
    unw_set_reg(&unw_cursor, UNW_ARM_R2,  signal_mcontext->arm_r2);
    unw_set_reg(&unw_cursor, UNW_ARM_R3,  signal_mcontext->arm_r3);
    unw_set_reg(&unw_cursor, UNW_ARM_R4,  signal_mcontext->arm_r4);
    unw_set_reg(&unw_cursor, UNW_ARM_R5,  signal_mcontext->arm_r5);
    unw_set_reg(&unw_cursor, UNW_ARM_R6,  signal_mcontext->arm_r6);
    unw_set_reg(&unw_cursor, UNW_ARM_R7,  signal_mcontext->arm_r7);
    unw_set_reg(&unw_cursor, UNW_ARM_R8,  signal_mcontext->arm_r8);
    unw_set_reg(&unw_cursor, UNW_ARM_R9,  signal_mcontext->arm_r9);
    unw_set_reg(&unw_cursor, UNW_ARM_R10, signal_mcontext->arm_r10);
    unw_set_reg(&unw_cursor, UNW_ARM_R11, signal_mcontext->arm_fp);
    unw_set_reg(&unw_cursor, UNW_ARM_R12, signal_mcontext->arm_ip);
    unw_set_reg(&unw_cursor, UNW_ARM_R13, signal_mcontext->arm_sp);
    unw_set_reg(&unw_cursor, UNW_ARM_R14, signal_mcontext->arm_lr);
    unw_set_reg(&unw_cursor, UNW_ARM_R15, signal_mcontext->arm_pc);

    unw_set_reg(&unw_cursor, UNW_REG_IP,  signal_mcontext->arm_pc);
    unw_set_reg(&unw_cursor, UNW_REG_SP,  signal_mcontext->arm_sp);

    // unw_step() does not return the first IP,
    // the address of the instruction which caused the crash.
    // Thus let's add this address manually.
    BacktraceState_AddAddress(state, signal_mcontext->arm_pc);

    //printf("unw_is_signal_frame(): %i\n", unw_is_signal_frame(&unw_cursor));

    // Unwind frames one by one, going up the frame stack.
    while (unw_step(&unw_cursor) > 0) {
        unw_word_t ip = 0;
        unw_get_reg(&unw_cursor, UNW_REG_IP, &ip);

        bool ok = BacktraceState_AddAddress(state, ip);
        if (!ok)
            break;

        //printf("unw_is_signal_frame(): %i\n", unw_is_signal_frame(&unw_cursor));
    }
}

#endif // #if LIBUNWIND_WITH_REGISTERS_METHOD


#if UNWIND_BACKTRACE_WITH_REGISTERS_METHOD

void ProcessRegisters(
        struct _Unwind_Context* unwind_context, BacktraceState* state) {
    assert(unwind_context);
    assert(state);

    const ucontext_t* signal_ucontext = state->signal_ucontext;
    assert(signal_ucontext);

    const struct sigcontext* signal_mcontext = &(signal_ucontext->uc_mcontext);
    assert(signal_mcontext);

    _Unwind_SetGR(unwind_context, REG_R0,  signal_mcontext->arm_r0);
    _Unwind_SetGR(unwind_context, REG_R1,  signal_mcontext->arm_r1);
    _Unwind_SetGR(unwind_context, REG_R2,  signal_mcontext->arm_r2);
    _Unwind_SetGR(unwind_context, REG_R3,  signal_mcontext->arm_r3);
    _Unwind_SetGR(unwind_context, REG_R4,  signal_mcontext->arm_r4);
    _Unwind_SetGR(unwind_context, REG_R5,  signal_mcontext->arm_r5);
    _Unwind_SetGR(unwind_context, REG_R6,  signal_mcontext->arm_r6);
    _Unwind_SetGR(unwind_context, REG_R7,  signal_mcontext->arm_r7);
    _Unwind_SetGR(unwind_context, REG_R8,  signal_mcontext->arm_r8);
    _Unwind_SetGR(unwind_context, REG_R9,  signal_mcontext->arm_r9);
    _Unwind_SetGR(unwind_context, REG_R10, signal_mcontext->arm_r10);
    _Unwind_SetGR(unwind_context, REG_R11, signal_mcontext->arm_fp);
    _Unwind_SetGR(unwind_context, REG_R12, signal_mcontext->arm_ip);
    _Unwind_SetGR(unwind_context, REG_R13, signal_mcontext->arm_sp);
    _Unwind_SetGR(unwind_context, REG_R14, signal_mcontext->arm_lr);
    _Unwind_SetGR(unwind_context, REG_R15, signal_mcontext->arm_pc);

    // Program Counter register aka Instruction Pointer will contain
    // the address of the instruction where the crash happened.
    // UnwindBacktraceCallback() will not supply us with it.
    BacktraceState_AddAddress(state, signal_mcontext->arm_pc);
}

_Unwind_Reason_Code UnwindBacktraceWithRegistersCallback(
        struct _Unwind_Context* unwind_context, void* state_voidp) {
    assert(unwind_context);
    assert(state_voidp);

    BacktraceState* state = (BacktraceState*)state_voidp;
    assert(state);

    // On the first UnwindBacktraceCallback() call,
    // set registers to _Unwind_Context and BacktraceState.
    if (state->address_count == 0) {
        ProcessRegisters(unwind_context, state);
        return _URC_NO_REASON;
    }

    uintptr_t ip = _Unwind_GetIP(unwind_context);
    bool ok = BacktraceState_AddAddress(state, ip);
    if (!ok)
        return _URC_END_OF_STACK;

    return _URC_NO_REASON;
}

void UnwindBacktraceWithRegisters(BacktraceState* state) {
    assert(state);
    _Unwind_Backtrace(UnwindBacktraceWithRegistersCallback, state);
}

#endif // #if UNWIND_BACKTRACE_WITH_REGISTERS_METHOD


#if UNWIND_BACKTRACE_WITH_SKIPPING_METHOD

_Unwind_Reason_Code UnwindBacktraceWithSkippingCallback(
        struct _Unwind_Context* unwind_context, void* state_voidp) {
    assert(unwind_context);
    assert(state_voidp);

    BacktraceState* state = (BacktraceState*)state_voidp;
    assert(state);

    // Skip some initial addresses, because they belong
    // to the signal handler frame.
    if (state->address_skip_count > 0) {
        state->address_skip_count--;
        return _URC_NO_REASON;
    }

    uintptr_t ip = _Unwind_GetIP(unwind_context);
    bool ok = BacktraceState_AddAddress(state, ip);
    if (!ok)
        return _URC_END_OF_STACK;

    return _URC_NO_REASON;
}

void UnwindBacktraceWithSkipping(BacktraceState* state) {
    assert(state);
    _Unwind_Backtrace(UnwindBacktraceWithSkippingCallback, state);
}

#endif // #if UNWIND_BACKTRACE_WITH_SKIPPING_METHOD


void PrintBacktrace(BacktraceState* state) {
    assert(state);

    size_t frame_count = state->address_count;
    for (size_t frame_index = 0; frame_index < frame_count; ++frame_index) {

        void* address = (void*)(state->addresses[frame_index]);
        assert(address);

        const char* symbol_name = "";

        Dl_info info = {};
        if (dladdr(address, &info) && info.dli_sname) {
            symbol_name = info.dli_sname;
        }

        // Relative address matches the address which "nm" and "objdump"
        // utilities give you, if you compiled position-independent code
        // (-fPIC, -pie).
        // Android requires position-independent code since Android 5.0.
        unsigned long relative_address = (char*)address - (char*)info.dli_fbase;

        char* demangled = NULL;

#if ENABLE_DEMANGLING
        int status = 0;
        demangled = __cxa_demangle(symbol_name, NULL, NULL, &status);
        if (demangled)
            symbol_name = demangled;
#endif

        assert(symbol_name);
        printf("  #%02zu:  0x%lx  %s\n", frame_index, relative_address, symbol_name);

        free(demangled);
    }
}

void SigActionHandler(int sig, siginfo_t* info, void* ucontext) {
    const ucontext_t* signal_ucontext = (const ucontext_t*)ucontext;
    assert(signal_ucontext);

#if LIBUNWIND_WITH_REGISTERS_METHOD
    {
        printf("Backtrace captured using LIBUNWIND_WITH_REGISTERS_METHOD:\n");
        BacktraceState backtrace_state;
        BacktraceState_Init(&backtrace_state, signal_ucontext);
        LibunwindWithRegisters(&backtrace_state);
        PrintBacktrace(&backtrace_state);
    }
#endif

#if UNWIND_BACKTRACE_WITH_REGISTERS_METHOD
    {
        printf("Backtrace captured using UNWIND_BACKTRACE_WITH_REGISTERS_METHOD:\n");
        BacktraceState backtrace_state;
        BacktraceState_Init(&backtrace_state, signal_ucontext);
        UnwindBacktraceWithRegisters(&backtrace_state);
        PrintBacktrace(&backtrace_state);

    }
#endif

#if UNWIND_BACKTRACE_WITH_SKIPPING_METHOD
    {
        printf("Backtrace captured using UNWIND_BACKTRACE_WITH_SKIPPING_METHOD:\n");
        BacktraceState backtrace_state;
        BacktraceState_Init(&backtrace_state, signal_ucontext);
        UnwindBacktraceWithSkipping(&backtrace_state);
        PrintBacktrace(&backtrace_state);

    }
#endif

    exit(0);
}


void SetUpAltStack() {
    // Set up an alternate signal handler stack.
    stack_t stack = {};
    stack.ss_size = 0;
    stack.ss_flags = 0;
    stack.ss_size = SIGSTKSZ;
    stack.ss_sp = malloc(stack.ss_size);
    assert(stack.ss_sp);

    sigaltstack(&stack, NULL);
}

void SetUpSigActionHandler() {
    // Set up signal handler.
    struct sigaction action = {};
    memset(&action, 0, sizeof(action));
    sigemptyset(&action.sa_mask);
    action.sa_sigaction = SigActionHandler;
    action.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;

    sigaction(SIGSEGV, &action, NULL);
}

void PreCrash3() {
}

void PreCrash2() {
    PreCrash3();
}

void PreCrash1() {
    PreCrash2();
}

// Prevent the compiler to "take advantage" of the Undefined Behavior
// in Crash() and generate "optimized" hard-crashing code that
// will prevent the signal handler to be called.
#if __clang__
void Crash() __attribute__((optnone));
#elif __GNUC__
void Crash() __attribute__((optimize("O0")));
#endif

void Crash() {
    PreCrash1();

    // Force crash.
    int* p = NULL;
    *p = 0;
}

void Func1() {
    Crash();
}

void Func2() {
    Func1();
}

void Func3() {
    Func2();
}
#endif

GTEST_API_ int main(int argc, char **argv) {
#ifdef __LINUX__
    signal(SIGSEGV, handler);
    signal(SIGABRT, handler);
#elif defined(__ANDROID__)
    SetUpAltStack();
    SetUpSigActionHandler();
#endif

    printf("Running main() from gtest_main.cpp\n");
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
