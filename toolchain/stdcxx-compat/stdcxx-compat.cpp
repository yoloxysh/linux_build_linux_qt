/* Call the board libstdc++ helper. Do not throw here: a local throw
   emits R_ARM_THM_MOVW_ABS_NC, which cannot go into a shared object. */
extern "C" void __cxa_throw_bad_array_new_length();
namespace std {
void __throw_bad_array_new_length() {
    __cxa_throw_bad_array_new_length();
    __builtin_unreachable();
}
}
