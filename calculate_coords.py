from math import degrees, radians, sin, cos, tan, asin, acos, atan2, pi
import time

f = open("star_info.txt")
star_info = f.readlines()[1:]
names = [line.split("\t")[1] for line in star_info]
#print(names)
coords = [tuple([float(num) for num in line.split("\t")[3:5]]) for line in star_info]
#print(coords)
star_coords = {names[i]: coords[i] for i in range(len(names))}
#print(star_coords)
f.close()

def julian_date(date = [2000, 1, 1, 0, 0]):
    # info from aa.usno.navy.mil/faq/docs/JulianDate.php, code by me
    
    year, month, day, hour, minute = date
    jan_1_2000 = 2451544.5
    months = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]
    days = (year - 2000) * 365 + (year - 1997) // 4
    days += sum([months[i] for i in range(month - 1)])
    if month > 2 and year % 4 == 0:
        days += 1
    days += (day - 1) + hour / 24 + minute / (24 * 60)
    return days + jan_1_2000

#print(julian_date([int(i) for i in input("Enter the calendar date: ").split()]))

def sidereal_time(julian_date):
    #formula from aa.usno.navy.mil/faq/docs/GAST.php

    sidereal_time = 18.697374558 + 24.06570982441908 * (julian_date - 2451545)
    sidereal_time %= 24
    #print("sidereal time:", sidereal_time)
    return sidereal_time

def sidereal_2(jd_since_2019):
    return (6.6907 + 24.0657098 * jd_since_2019) % 24

def jd_since_2019(date = [2019, 1, 1, 0, 0]):
    year, month, day, hour, minute = date
    months = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]
    days = (year - 2019) * 365 + (year - 2017) // 4
    days += sum([months[i] for i in range(month - 1)])
    if month > 2 and year % 4 == 0:
        days += 1
    days += (day - 1) + hour / 24 + minute / (24 * 60)
    return days

#print(sidereal_time(float(input())))
    
def calculate_coords(ras, dec, lat, lon, date):
    #formulae from aa.usno.navy.mil/faq/docs/Alt_Az.php

    LHA = radians((sidereal_time(julian_date(date)) - ras * 24 / 360) * 15 + lon)
    lat = radians(lat)
    dec = radians(dec)

    alt = degrees(asin(cos(LHA) * cos(dec) * cos (lat) + sin(dec) * sin(lat)))
    azi = degrees(atan2(-sin(LHA), tan(dec) * cos(lat) - sin(lat) * cos(LHA)))
    azi %= 360

    return round(alt, 1), round(azi, 1)

def ang_dist(current_coords, target_coords):
    #formula from spiff.rit.edu/classes/phys373/lectures/radec/radec.html
    
    alt_0, azi_0, alt_1, azi_1 = [radians(angle) for angle in current_coords + target_coords]

    ang_dist = acos(cos(radians(90) - alt_0) * cos(radians(90) - alt_1) +
                    sin(radians(90) - alt_0) * sin(radians(90) - alt_1) *
                    cos(azi_0 - azi_1))
    return round(degrees(ang_dist), 2)

def find_closest(current_coords, date = [2020, 1, 1, 0, 0]):
    lon = - dms_to_deg(78, 52, 56.64)
    lat = dms_to_deg(38, 28, 17.04)
    min_dist = 180
    closest = "Tralfamadore"
    closest_coords = (0, 0)
    for name, (ras, dec) in star_coords.items():
        target_coords = calculate_coords(ras, dec, lat, lon, date)
        dist = ang_dist(current_coords, target_coords)
        if dist < min_dist:
            print(dist)
            min_dist = dist
            closest = name
            closest_coords = target_coords
    return closest, closest_coords, min_dist
    

def dms_to_deg(degrees, minutes, seconds):
    return degrees + minutes / 60 + seconds / 3600

def hms_to_deg(hours, minutes, seconds):
    return (hours + minutes / 60 + seconds / 3600) / 24 * 360

def test1():
    num_intvs = 1
    intv_unit = 2
    # 1 = minute, 2 = hour, 3 = day
    intv_length = 1
    start_date = [2019, 11, 23, 21, 45]
    lon = -78
    lat = 38
    for name, (ras, dec) in star_coords.items():
        date = start_date[:]
        print("\n" + name + ":")
        for i in range(num_intvs):
            print("-".join([str(num) for num in date[:3]]),
                  ":".join([str(num) for num in date[3:]]), end = ": ")
            print(calculate_coords(ras, dec, lat, lon, date))
            date[5 - intv_unit] += intv_length

def test2():
    while not input("Press enter to continue"):
        current_coords = tuple([float(num) for num in input("enter current coords: ").split()])
        target_coords = tuple([float(num) for num in input("enter target coords: ").split()])
        print("angle between:", ang_dist(current_coords, target_coords))


def test3():
    while True:
        current_coords = tuple([float(num) for num in
                                input("enter current coords: ").split()])
        if current_coords == (66, 66):
            break
        closest, closest_coords, min_dist = find_closest(current_coords)
        min_dist = round(min_dist, 1)
        if min_dist < 1:
            print("You're pointing right at %s! (%.1f degrees)" % (closest, min_dist))
        else:
            plural = "" if min_dist == 1 else "s"
            print("%s %s is only %.f degree%s away!" % (closest, closest_coords,
                                                        min_dist, plural))

def test4():
    for lat, lon in star_coords.values():
        print("{%.4f, %.4f}," % (lat, lon / 57.2958))

def test5():
    for name in names:
        print('"%s",' % name, end = " ")

def test6():
    date = [2019, 11, 23, 21, 45]
    alt = 19
    azi = 126
    lat = 38
    lon = -76
    print(find_closest((alt, azi), date))
    

test6()
