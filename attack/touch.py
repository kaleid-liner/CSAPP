import sys

phase = int(sys.argv[1])

with open(f'touch{ phase }.txt', 'w') as out:
    out.write('00 ' * 40)
    
    if phase == 1:
        out.write('C0 17 40 ' + '00 ' * 7 + '\n')

    if phase == 4:
        out.write('AB 19 40 00 ' + '00 ' * 4
                + 'FA 97 B9 59 ' + '00 ' * 4
                + 'A2 19 40 00 ' + '00 ' * 4
                + 'EC 17 40 00 ' + '00 ' * 4 + '\n');


