import sys

with open('touch1.txt', 'w') as out:
    out.write('00 ' * 40)
    phase = int(sys.argv[1])
    
    if phase == 1:
        out.write('C0 17 40 ' + '00 ' * 7 + '\n')

