; dolly-6502 hello world example

PRINT_ADDR = $0500
PRINT_ADDR_HI = $05
PRINT_ADDR_LO = $00

.text "syscalls"
.org $8800
print:
    lda #PRINT_ADDR_HI
    sta $FF
    lda #PRINT_ADDR_LO
    sta $FE
    lda #$01
    brk
    rts
exit:
    lda #$00
    brk
    rts

.text "_start"
.org $8000

main:
    ldx #$ff
_main_loop:
    inx
    lda greeting,x
    sta PRINT_ADDR,x
    bne _main_loop
    jsr print
    jsr exit

.data "strings"
greeting: .string "hello world!\n"

.text "__interrupt"
.org $FF7F
interrupt: rti

.data "__interrupt_vector"
.org $FFFE
.word $FF7F
