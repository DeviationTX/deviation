
//Accessor to reset static variable
u8 TEST_Button_InterruptLongPress()
{
   u8 old = interrupt_longpress;
   interrupt_longpress = 0;
   return old;
}
