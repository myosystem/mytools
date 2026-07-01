SECTIONS
{
    . = 0x400000;

    .text : {
        KEEP(*(.entry))
        *(.text*)
    } :text

    .rodata : {
        *(.rodata*)
    } :rodata

    .data : {
        *(.data*)
        *(.bss*)
        *(COMMON)
    } :data
}
PHDRS {
  text   PT_LOAD FLAGS(5);
  rodata PT_LOAD FLAGS(4);
  data   PT_LOAD FLAGS(6);
}
