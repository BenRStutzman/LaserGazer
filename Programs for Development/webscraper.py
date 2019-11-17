import requests
import urllib.request
import time
import socket

bodies = {
    ##"mercury": 1,
    "venus": 2,
    "mars": 4,
    "jupiter": 5,
    "saturn": 6,
    ##"uranus": 7,
    ##"neptune": 8,
    ##"pluto": 9,
    "sun": 10,
    "moon": 11,
    ##"achernar": -1,
    "adhara": -2,
    "aldebaran": -3,
    "altair": -4,
    "antares": -5,
    "arcturus": -6,
    "betelgeuse": -7,
    ##"canopus": -8,
    "capella": -9,
    "deneb": -10,
    "fomalhaut": -11,
    ##"hadar": -12,
    ##"mimosa": -13,
    "polaris": -14,
    "pollux": -15,
    "procyon": -16,
    "regulus": -17,
    "rigel": -18,
    ##"rigil kentaurus": -19,
    "sirius": -20,
    "spica": -21,
    "vega": -22
    }

intv_units = {
    "day": 1,
    "hour": 2,
    "minute": 3,
    "second": 4
    }

places = {
    "emu hill": [-1, 78, 52, 56.64, 1, 38, 28, 17.04, 481]
    }

months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun",
          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]


def get_data(body_name = "", form = 2, ID = "AA", task = 9, body = -14,
             year = 2020, month = 1, day = 1, hr = 0, minute = 0, sec = 0.0,
             intv_mag = 1, intv_unit = 3, reps = 10, place = "emu hill",
             lon_sign = -1, lon_deg = 78, lon_min = 52, lon_sec = 56.64,
             lat_sign = 1, lat_deg = 38, lat_min = 28, lat_sec = 17.04,
             height= 481):

    if body_name:
        body = bodies[body_name.lower()]

    url = "https://aa.usno.navy.mil/cgi-bin/aa_topocentric2.pl?form=" + str(form) + "&ID=" + ID + "&task=" + str(task) + "&body=" + str(body) + "&year="
    url += str(year) + "&month=" + str(month) + "&day=" + str(day) + "&hr=" + str(hr) + "&min=" + str(minute) + "&sec=" + str(sec) + "&intv_mag="
    url += str(intv_mag) + "&intv_unit=" + str(intv_unit) + "&reps=" + str(reps) + "&place=" + place + "&lon_sign=" + str(lon_sign) + "&lon_deg="
    url += str(lon_deg) + "&lon_min=" + str(lon_min) + "&lon_sec=" + str(lon_sec) + "&lat_sign=" + str(lat_sign) + "&lat_deg=" + str(lat_deg)
    url += "&lat_min=" + str(lat_min) + "&lat_sec=" + str(lat_sec) + "&height=" + str(height)

    #print(url)
    attempt = 1
    response = ""
    while attempt <= 10 and str(response) != "<Response [200]>":
        #print("Attempt %d:" % attempt)
        try:
            response = requests.get(url)
            #print("Successful connection")
            #time.sleep(1)
        except socket.gaierror:
            print("Connection failed for body %d on attempt %d" % (body, attempt))
        attempt += 1
    #time.sleep(1)
    page = response.text
    #print(page)

    name = page.splitlines()[25].strip()
    start_loc = page.find(str(year))
    end_loc = page.find("</pre>")
    chart = page[start_loc:end_loc]
    #print(chart)
    lines = [line.split() for line in chart.splitlines()]

    data = {}

    for line in lines:
        tm = (int(line[0]), months.index(line[1]) + 1, int(line[2]))
        tm += tuple([int(num) for num in line[3].split(":")[:2]])
        altitude = round(90 - (int(line[4]) + int(line[5]) / 60 + float(line[6]) / 3600), 2)
        azimuth = round(int(line[7]) + int(line[8]) / 60 + float(line[9]) / 3600, 2)
        data[tm] = (altitude, azimuth)

    #print("Coordinates of %s starting at:" % name, lines[0][0:4])
    return data

def get_all_data(time_name = "", place_name = "", form = 2, ID = "AA", task = 9,
                year = 2020, month = 1, day = 1, hr = 0, minute = 0, sec = 0.0,
                intv_mag = 1, intv_unit = 3, reps = 10, place = "emu hill",
                lon_sign = -1, lon_deg = 78, lon_min = 52, lon_sec = 56.64,
                lat_sign = 1, lat_deg = 38, lat_min = 28, lat_sec = 17.04,
                height = 481):

    if time_name:
        if time_name.lower() == "this evening":
            now = time.localtime()
            year, month, day = now.tm_year, now.tm_mon, now.tm_mday
            hr, minute, sec = 21, 0, 0
        elif time_name.lower() == "now":
            now = time.gmtime()
            year, month, day = now.tm_year, now.tm_mon, now.tm_mday
            hr, minute, sec = now.tm_hour, now.tm_min, 0
    if place_name:
        place = place_name
        coords = places[place_name.lower()]
        lon_sign, lon_deg, lon_min, lon_sec = coords[:4]
        lat_sign, lat_deg, lat_min, lat_sec = coords[4:8]
        height = coords[8]
    
    all_data = {}        

    print("Getting celestial locations from our friend the Navy...\n")
    time.sleep(1)
    for body_name, body in bodies.items():
        print("\n" + body_name[0].upper() + body_name[1:] + ":")
        data = get_data("", 2, "AA", 9, body,
                   year, month, day, hr, minute, sec, intv_mag, intv_unit, reps,
                   place, lon_sign, lon_deg, lon_min, lon_sec, lat_sign, lat_deg,
                   lat_min, lat_sec, height)
        print(data[(year, month, day, hr, minute)])
        for tm, coords in data.items():
            #print(coords)
            if tm in all_data:
                all_data[tm][body] = coords
            else:
                all_data[tm] = {body: coords}
    return all_data

def write_to_file(all_data, file_name = "stardata.txt"):    
    file = open("stardata.txt", "w")
    line = "time" + " " * 17
    for body_name in bodies.keys():
        line += (body_name[0].upper() + body_name[1:]).ljust(16, " ")
    file.write(line + "\n")

    for tm, coords in all_data.items():
        line = "-".join([str(num).rjust(2, " ") for num in tm[:3]])
        line += " " + ":".join([str(num).rjust(2, " ") for num in tm[3:]])
        for alt, azi in coords.values():
            line += "   " + str("%.1f" % alt).rjust(6, " ")
            line += " " + str("%.1f" % azi).rjust(6, " ")
        file.write(line + "\n")
    file.close()

    print("\nLocations added to file: " + file_name)
    
def test():
    all_data = get_all_data(year = 2020, month = 4, day = 22, hr = 16, minute = 30,
                            height = 0, reps = 10, intv_unit = 2)
    write_to_file(all_data)

def test2():
    print(get_data(body_name = "capella", year = 2025, month = 4, day = 22,
                   hr = 16, minute = 45, sec = 0))

test()


