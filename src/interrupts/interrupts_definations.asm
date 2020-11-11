; cf. https://github.com/ghaiklor/ghaiklor-os-gcc
; cf. https://github.com/tmathmeyer/sos
global interrupts

extern intr_isr_handler
extern intr_irq_handler

%macro def_isr_handler 1
    global isr%1
    isr%1:
        cli
        mov rdi, dword %1
        jmp isr_common_stub
%endmacro

%macro def_irq_handler 1
    global irq%1
    irq%1:
        cli
        mov rdi, dword (32 + %1)
        jmp irq_common_stub
%endmacro

isr_common_stub:
    ; save registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rsi, rsp

    ; call handler
    call intr_isr_handler

    ; restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    sti
    iretq

; define interruptions
; should be keep in sync with src/core/isr.h
def_isr_handler 0
def_isr_handler 1
def_isr_handler 2
def_isr_handler 3
def_isr_handler 4
def_isr_handler 5
def_isr_handler 6
def_isr_handler 7
def_isr_handler 8
def_isr_handler 9
def_isr_handler 10
def_isr_handler 11
def_isr_handler 12
def_isr_handler 13
def_isr_handler 14
def_isr_handler 15
def_isr_handler 16
def_isr_handler 17
def_isr_handler 18
def_isr_handler 19
def_isr_handler 20
def_isr_handler 21
def_isr_handler 22
def_isr_handler 23
def_isr_handler 24
def_isr_handler 25
def_isr_handler 26
def_isr_handler 27
def_isr_handler 28
def_isr_handler 29
def_isr_handler 30
def_isr_handler 31

def_isr_handler 37
def_isr_handler 38
def_isr_handler 39
def_isr_handler 40
def_isr_handler 41
def_isr_handler 42
def_isr_handler 43
def_isr_handler 44
def_isr_handler 45
def_isr_handler 46
def_isr_handler 47
def_isr_handler 48
def_isr_handler 49
def_isr_handler 50
def_isr_handler 51
def_isr_handler 52
def_isr_handler 53
def_isr_handler 54
def_isr_handler 55
def_isr_handler 56
def_isr_handler 57
def_isr_handler 58
def_isr_handler 59
def_isr_handler 60
def_isr_handler 61
def_isr_handler 62
def_isr_handler 63
def_isr_handler 64
def_isr_handler 65
def_isr_handler 66
def_isr_handler 67
def_isr_handler 68
def_isr_handler 69
def_isr_handler 70
def_isr_handler 71
def_isr_handler 72
def_isr_handler 73
def_isr_handler 74
def_isr_handler 75
def_isr_handler 76
def_isr_handler 77
def_isr_handler 78
def_isr_handler 79
def_isr_handler 80
def_isr_handler 81
def_isr_handler 82
def_isr_handler 83
def_isr_handler 84
def_isr_handler 85
def_isr_handler 86
def_isr_handler 87
def_isr_handler 88
def_isr_handler 89
def_isr_handler 90
def_isr_handler 91
def_isr_handler 92
def_isr_handler 93
def_isr_handler 94
def_isr_handler 95
def_isr_handler 96
def_isr_handler 97
def_isr_handler 98
def_isr_handler 99
def_isr_handler 100
def_isr_handler 101
def_isr_handler 102
def_isr_handler 103
def_isr_handler 104
def_isr_handler 105
def_isr_handler 106
def_isr_handler 107
def_isr_handler 108
def_isr_handler 109
def_isr_handler 110
def_isr_handler 111
def_isr_handler 112
def_isr_handler 113
def_isr_handler 114
def_isr_handler 115
def_isr_handler 116
def_isr_handler 117
def_isr_handler 118
def_isr_handler 119
def_isr_handler 120
def_isr_handler 121
def_isr_handler 122
def_isr_handler 123
def_isr_handler 124
def_isr_handler 125
def_isr_handler 126
def_isr_handler 127

; That is for syscalls
def_isr_handler 128

def_isr_handler 129
def_isr_handler 130
def_isr_handler 131
def_isr_handler 132
def_isr_handler 133
def_isr_handler 134
def_isr_handler 135
def_isr_handler 136
def_isr_handler 137
def_isr_handler 138
def_isr_handler 139
def_isr_handler 140
def_isr_handler 141
def_isr_handler 142
def_isr_handler 143
def_isr_handler 144
def_isr_handler 145
def_isr_handler 146
def_isr_handler 147
def_isr_handler 148
def_isr_handler 149
def_isr_handler 150
def_isr_handler 151
def_isr_handler 152
def_isr_handler 153
def_isr_handler 154
def_isr_handler 155
def_isr_handler 156
def_isr_handler 157
def_isr_handler 158
def_isr_handler 159
def_isr_handler 160
def_isr_handler 161
def_isr_handler 162
def_isr_handler 163
def_isr_handler 164
def_isr_handler 165
def_isr_handler 166
def_isr_handler 167
def_isr_handler 168
def_isr_handler 169
def_isr_handler 170
def_isr_handler 171
def_isr_handler 172
def_isr_handler 173
def_isr_handler 174
def_isr_handler 175
def_isr_handler 176
def_isr_handler 177
def_isr_handler 178
def_isr_handler 179
def_isr_handler 180
def_isr_handler 181
def_isr_handler 182
def_isr_handler 183
def_isr_handler 184
def_isr_handler 185
def_isr_handler 186
def_isr_handler 187
def_isr_handler 188
def_isr_handler 189
def_isr_handler 190
def_isr_handler 191
def_isr_handler 192
def_isr_handler 193
def_isr_handler 194
def_isr_handler 195
def_isr_handler 196
def_isr_handler 197
def_isr_handler 198
def_isr_handler 199
def_isr_handler 200
def_isr_handler 201
def_isr_handler 202
def_isr_handler 203
def_isr_handler 204
def_isr_handler 205
def_isr_handler 206
def_isr_handler 207
def_isr_handler 208
def_isr_handler 209
def_isr_handler 210
def_isr_handler 211
def_isr_handler 212
def_isr_handler 213
def_isr_handler 214
def_isr_handler 215
def_isr_handler 216
def_isr_handler 217
def_isr_handler 218
def_isr_handler 219
def_isr_handler 220
def_isr_handler 221
def_isr_handler 222
def_isr_handler 223
def_isr_handler 224
def_isr_handler 225
def_isr_handler 226
def_isr_handler 227
def_isr_handler 228
def_isr_handler 229
def_isr_handler 230
def_isr_handler 231
def_isr_handler 232
def_isr_handler 233
def_isr_handler 234
def_isr_handler 235
def_isr_handler 236
def_isr_handler 237
def_isr_handler 238
def_isr_handler 239
def_isr_handler 240
def_isr_handler 241
def_isr_handler 242
def_isr_handler 243
def_isr_handler 244
def_isr_handler 245
def_isr_handler 246
def_isr_handler 247
def_isr_handler 248
def_isr_handler 249
def_isr_handler 250
def_isr_handler 251
def_isr_handler 252
def_isr_handler 253
def_isr_handler 254
def_isr_handler 255

irq_common_stub:
    ; save registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rsi, rsp

    ; call handler
    call intr_irq_handler

    ; restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    sti
    iretq

; define hardware interruptions
; should be keep in sync with src/core/isr.h
def_irq_handler 0
def_irq_handler 1
def_irq_handler 2
def_irq_handler 3
def_irq_handler 4
