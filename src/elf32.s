; elf32.s: header, boot ritual and other things libc refused to do.
; the ELF and program headers share rent. the stack becomes the screen.
; fd numerology after the smoke clears:
;   0 keyboard, 3 pixels, 4 clock, 6 bytebeat pipe.
; disturb the open order and demo.c starts mailing pixels to strangers.

bits 32

%ifndef width
    %define width 1024
%endif
%ifndef height
    %define height 768
%endif

; pick the real pixel hole or its desktop cosplay
%ifdef USE_TMPFB
    %define FBDEV "/tmp/fb0"
%else
    %define FBDEV "/dev/fb0"
%endif

screen_size equ width * height * 4

; two headers enter, one tiny pile of bytes leaves
org 0x00010000

ehdr:
    db 0x7F, "ELF"          ; passport, somehow valid
    dd 1                    ; class padding moonlights as PT_LOAD
    dd 0                    ; start reading at the beginning. revolutionary!
    dd $$                   ; live at 0x10000 where rent is cheap
    dw 2                    ; executable, allegedly
    dw 3                    ; i386, forever young
    dd code_start           ; filesize and version share a toothbrush
    dd code_start           ; entry and memsize do the same
    dd 4                    ; readable. execution sneaks in anyway

fname:
    db FBDEV, 0             ; section headers were overrated

code_start:
    ; this mov also pretends to be phentsize=32 and phnum=1
    mov ebx, fname

    ; keep argv around so aplay can inherit the outside world
    mov ebp, esp

%ifdef CLOSE_FD3
    ; vondehi squats on sacred fd 3. evict it
    push ebx
    mov al, 6
    push byte 3
    pop ebx
    int 0x80
    pop ebx
%endif

    ; ask the pixel hole to accept our mistakes
    xor ecx, ecx
    inc ecx
    mov al, 5
    int 0x80

    ; rent a few megabytes from the stack. what could go wrong
    sub esp, screen_size

    ; stop the terminal from "helping" with keys
    sub esp, 64

    xor eax, eax
    mov al, 54
    xor ebx, ebx
    mov cx, 0x5401
    mov edx, esp
    int 0x80

    and byte [esp + 12], 0xF5    ; no canon, no echo, no manners

    inc ecx
    mov al, 54
    int 0x80

    ; keyboard may answer later. pixels have deadlines
    mov al, 55
    push byte 4
    pop ecx
    xor edx, edx
    mov dh, 8
    int 0x80

    ; government-approved 50 Hz attention span
    mov ax, 0x142
    xor ecx, ecx
    int 0x80

    mov ebx, eax
    mov eax, 20000000
    push eax
    push ecx
    push eax
    push ecx
    mov eax, 0x145
    mov edx, esp
    int 0x80

    ; hire a child to scream bytes into aplay
    sub esp, 8
    mov al, 0x2A
    mov ebx, esp
    int 0x80

    push dword [esp + 4]
    mov al, 2
    int 0x80
    dec eax
    js .child

    ; parent keeps the noisy end
    pop ebx

%ifndef CLOSE_FD3
 %ifndef NO_SIGPIPE
    ; if the band leaves, keep drawing like nothing happened
    xor eax, eax
    mov al, 48
    push byte 13
    pop ebx
    push byte 1
    pop ecx
    int 0x80
 %endif
%endif

    ; sweep timer, tty and pipe paperwork off the pixels
    add esp, 88

    jmp .init_done

.child:
    ; child turns pipe into stdin and becomes aplay
    pop ecx
    inc eax

    mov al, 0x3F
    pop ebx
    xor ecx, ecx
    int 0x80

    push eax
    lea edx, [ebp + 12]
    mov ebx, aplay_path
    lea ecx, [ebx + 5]
    push ecx
    mov ecx, esp
    mov al, 0x0B
    int 0x80

.init_done:
    ; hand the wall to C. ebx is the only map it gets
    mov ebx, esp

incbin "payload.bin"

; nine expensive bytes for the band
aplay_path:
    db "/bin/aplay", 0

filesize equ $ - ehdr
