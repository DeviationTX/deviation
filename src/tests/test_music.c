#include "CuTest.h"
struct NoteMap {    
    const char str[4];  
    u16 note;   
};  
static const struct NoteMap note_map[] = {  
{"xx",   0}, {"a",  220}, {"ax", 233}, {"b",  247}, 

{"c0", 262}, {"cx0",277}, {"d0", 294}, {"dx0",311}, {"e0", 330}, {"f0", 349},  
{"fx0",370}, {"g0", 392}, {"gx0",415}, {"a0", 440}, {"ax0",466}, {"b0", 494},  

{"c1", 523}, {"cx1",554}, {"d1", 587}, {"dx1",622}, {"e1", 659}, {"f1", 698}, 
{"fx1",740}, {"g1", 784}, {"gx1",831}, {"a1", 880}, {"ax1",932}, {"b1", 988}, 

{"c2", 1047},{"cx2",1109},{"d2", 1175},{"dx2",1245},{"e2", 1319},{"f2", 1397},   
{"fx2",1480},{"g2", 1568},{"gx2",1661},{"a2", 1760},{"ax2",1865},{"b2", 1976},   

{"c3", 2093},{"cx3",2217},{"d3", 2349},{"dx3",2489},{"e3", 2637},{"f3", 2794},  
{"fx3",2960},{"g3", 3136},{"gx3",3322},{"a3", 3520},{"ax3",3729},{"b3", 3951},  

{"c4", 4186},{"cx4",4435},{"d4", 4699},{"dx4",4978},{"e4", 5274},{"f4", 5588}, 
{"fx4",5920},{"g4", 6272},{"gx4",6645},{"a4", 7040},{"ax4",7459},{"b4", 7902}, 
};
#define NUM_NOTES (sizeof(note_map) / sizeof(struct NoteMap))

void TestMusic(CuTest *t)
{
    for (size_t i = 0;  i < NUM_NOTES; i++)
        CuAssertIntEquals(t, get_note(note_map[i].str), i);

    for (size_t i = 1;  i < NUM_NOTES; i++) {
        CuAssertTrue(t, abs(get_freq(i) - note_map[i].note) < 8);
    }
}
