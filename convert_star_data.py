f = open("star_info_north.txt")
star_info = f.readlines()
names = [line.split("\t")[0] for line in star_info]
#print(names)
mags = [line.split("\t")[1] for line in star_info]
#print(mags)
coords = [tuple([float(num) for num in line.split("\t")[2:4]]) for line in star_info]
#print(coords)
star_coords = {names[i]: coords[i] for i in range(len(names))}
#print(star_coords)
f.close()

for lat, lon in star_coords.values():
    print("{%.4f, %.4f}," % (lat, lon / 57.2958))

for i in range(len(names)):
    print('case %d: closest_name = "%s";' % (i, names[i]), end = " " * (16 - len(names[i]) + 1 - len(str(i))))
    print("break;")
