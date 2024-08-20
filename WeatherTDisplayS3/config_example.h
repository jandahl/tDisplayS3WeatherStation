#ifndef CONFIG_H
#define CONFIG_H

// Rename this file to config.h
// stash in your API key
// change the towns 
// correct the ntp server
//#################### EDIT THIS  ###################
#define OPENWEATHER_API_KEY "f00bar"

#define TOWN_COUNT 3
// Only Danish towns in the example
const char* towns[TOWN_COUNT] = {"Copenhagen", "Oslo", "Malmø"};

#define NTPSERVER "dk.pool.ntp.org"
#define WEATHERSERVER "https://api.openweathermap.org/data/2.5/weather?q="

#define TIMEZONE 2
#define METRICORIMPERIAL "metric"
//#################### end of edits ###################

#endif // CONFIG_H
