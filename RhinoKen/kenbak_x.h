
// Marcel Timm, RhinoDevel, 2024jul09

#ifndef KENBAK_X
#define KENBAK_X

// See page 06 of the logic schematics.
//
enum kenbak_x
{
    kenbak_x_none = 0, // None of the signals/states below was detected last.
    kenbak_x_1 = 1, // If signal DA was last. <=> Display address mode.
    kenbak_x_2 = 2, // If signal DD was last. <=> Display memory mode.
    kenbak_x_3 = 3, // If state is SA. <=> "Run mode" (see page 06!).
    kenbak_x_4 = 4 // If signals BU or CL were last. <=> Input mode.
};

#endif //KENBAK_X
