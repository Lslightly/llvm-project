.global alloc_enter         # 声明函数alloc_enter为全局可见
.type alloc_enter, @function # 声明alloc_enter是一个函数

alloc_enter:
    movq TLS_GRP@GOTTPOFF(%rip), %rax   # initial-exec: 拿到 TLS 偏移
    addq %fs:0, %rax                    # 加上线程 TLS 基址 (在 fs 段)
    movq %rdi, (%rax)                   # 把 rdi 写入 TLS_GRP
    ret

.global alloc_exit
.type alloc_exit, @function

alloc_exit:
    movq TLS_GRP@GOTTPOFF(%rip), %rax   # initial-exec: 拿到 TLS 偏移
    addq %fs:0, %rax                    # 加上线程 TLS 基址 (在 fs 段)
    movq $0, (%rax)                   # 把 rdi 写入 TLS_GRP
    ret
