    .section .rodata
    .global _font_data
    .align  4
_font_data:
    .incbin "digital-7 (mono).ttf"
_font_data_end:
    .global _font_data_size
    .align  4
_font_data_size:
    .int _font_data_end - _font_data
