const int num_bodies = 26;
int closest;
float closest_dist;
float sidereal;
float lat;
float lon;
float alt;
float azi;
float coords[num_bodies][2];

String names[num_bodies] = {"Polaris", "Sirius", "Canopus", "Rigil Kentaurus", "Arcturus",
                            "Vega", "Capella", "Rigel", "Procyon", "Achernar", "Betelgeuse",
                            "Hadar", "Acrux", "Altair", "Aldebaran", "Antares", "Spica", "Pollux",
                            "Fomalhaut", "Mimosa", "Deneb", "Regulus", "Adhara", "Castor", "Gacrux",
                            "Shaula"
                           };

float celes[num_bodies][2] = {  {37.9545, 1.5580},    // right ascension in degrees
                                {101.2870, -0.2668},  // and declination in radians
                                {95.9880, -0.8954},   // of all celestial bodies
                                {219.8995, -1.0326},
                                {213.9153, 0.3348},
                                {279.2347, 0.6769},
                                {79.1723, 0.8028},
                                {78.6386, -0.1361},
                                {114.8255, 0.0912},
                                {24.4286, -0.9907},
                                {88.7932, 0.1293},
                                {210.9559, 1.0537},
                                {186.6496, 1.1013},
                                {297.6958, 0.1548},
                                {68.9802, 0.2881},
                                {247.3518, -0.4462},
                                {201.2983, -0.1892},
                                {116.3289, 0.4891},
                                {344.4127, -0.4953},
                                {191.9303, -1.0177},
                                {310.3580, 0.7903},
                                {152.0930, 0.2089},
                                {104.6565, -0.4717},
                                {113.6500, 0.5566},
                                {187.7915, -0.9929},
                                {263.4022, -0.6440}, };

void calc_sidereal(int year, int month, int day, int hour, int minute) {
  // formula from https://aa.usno.navy.mil/faq/docs/GAST.php

  float days = (year - 2019) * 365 + (year - 2017) / 4;               // days in full years since 1/1/19
  int months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};  // lengths of months
  for (int i = 0; i < month - 1; i++) { days += months[i]; }          // add days in full months
  if (month > 2 && year % 4 == 0) {days += 1; }                       // add day from a leap year
  days += (day - 1) + hour / 24.0 + minute / 1440.0;                  // add days from this month
  sidereal = fmod(6.6907 + 24.0657 * days, 24);                       // sidereal time in hours
}

void calc_coords() {
  // formulae from https://aa.usno.navy.mil/faq/docs/Alt_Az.php
  
  for (int i = 0; i < num_bodies; i++) {
    float LHA = (sidereal * 15 - celes[i][0] + lon) / 57.2958;                // local hour angle
    float lat_rad = lat / 57.2958;                                            // lat in radians
    float dec = celes[i][1];                                                  // declination in radians

    coords[i][0] = asin(cos(LHA) * cos(dec) * cos(lat_rad) + sin(dec) * sin(lat_rad)) * 57.2958;  // altitude of the body
    coords[i][1] = fmod(atan2(-sin(LHA), tan(dec) * cos(lat_rad) - sin(lat_rad) * cos(LHA))       // azimuth of the body
                        * 57.2958 + 360, 360);
  }
}

void find_closest() {
  // formula from http://spiff.rit.edu/classes/phys373/lectures/radec/radec.html
  float alt_rad = alt / 57.2958;                        // convert to radians
  float azi_rad = azi / 57.2958;  
  float min_dist = 3.1416;                              // reset closest and min_dist
  closest = -1;                                         
  

  for (int i = 0; i < num_bodies; i++) {
    float targ_alt_rad = coords[i][0] / 57.2958;                            // target coords in radians
    float targ_azi_rad = coords[i][1] / 57.2958;
    float dist = acos(cos(1.5708 - alt_rad) * cos(1.5708 - targ_alt_rad) +  // angular distance
                      sin(1.5708 - alt_rad) * sin(1.5708 - targ_alt_rad) *
                      cos(azi_rad - targ_azi_rad));
    if (dist < min_dist) {                                                  // set closest to this one if it's closer
      Serial.println(dist);
      min_dist = dist;
      closest = i;
    }
  }
  closest_dist = min_dist * 57.2958;                                        // convert to degrees
}

void setup() {
  lat = 38;
  lon = -78;
  alt = 19;
  azi = 126;
  Serial.begin(9600);
  calc_sidereal(2019, 11, 23, 21, 45);
  calc_coords();
  find_closest();
  Serial.print(names[closest]); Serial.print(" "); Serial.println(closest_dist);
  Serial.print(coords[closest][0]); Serial.print(" "); Serial.print(coords[closest][1]);
}

void loop() {


}
