#![no_std]
#![feature(lang_items)]
/*
const GPIO_PIN_6 : u16 = 0x0040;
const GPIO_PIN_7 : u16 = 0x0080;

const PERIPH_BASE : u32 = 0x40000000;
const AHBPERIPH_BASE : u32 = PERIPH_BASE + 0x00020000;

const GPIOB_BASE : u32 = AHBPERIPH_BASE + 0x00000400;

const GPIOB : *mut GPIO_TypeDef = GPIOB_BASE as *mut GPIO_TypeDef;
*/

#[no_mangle]
pub extern "C" fn rust_main() -> i32 {
    
    return 3;

}



#[lang="eh_personality"] extern fn eh_personality() {}

#[lang="panic_fmt"]
extern fn panic_fmt(_: ::core::fmt::Arguments, _: &'static str, _: u32) -> ! {
    loop {}
}