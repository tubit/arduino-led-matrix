#ifndef EFFECTS_H
#define EFFECTS_H
#include <Arduino.h>
#include <MD_MAX72xx.h>

#define DELAYTIME 100 // in milliseconds
#define MAX_DEVICES 4

void bullseye(MD_MAX72XX *mx);
void stripe(MD_MAX72XX *mx);
void spiral(MD_MAX72XX *mx);
void bounce(MD_MAX72XX *mx);
void rows(MD_MAX72XX *mx);
void checkboard(MD_MAX72XX *mx);
void columns(MD_MAX72XX *mx);
void cross(MD_MAX72XX *mx);
void showCharset(MD_MAX72XX *mx);
void transformation1(MD_MAX72XX *mx);
void transformation2(MD_MAX72XX *mx);
void intensity(MD_MAX72XX *mx);
void blinking(MD_MAX72XX *mx);

#endif