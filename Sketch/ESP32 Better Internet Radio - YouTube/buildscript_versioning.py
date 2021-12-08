import datetime
tm = datetime.datetime.today()

FILENAME_BUILDNO = 'versioning.txt'
FILENAME_VERSION_H = 'include/version.h'
version = 'v1.' + str(tm.year)[-2:]+('0'+str(tm.month))[-2:]+('0'+str(tm.day))[-2:] + '_'
shortversion = 'v1.'

build_no = 0
try:
    with open(FILENAME_BUILDNO) as f:
        build_no = int(f.readline()) + 1
except:
    print('Starting build number from 1..')
    build_no = 1
with open(FILENAME_BUILDNO, 'w+') as f:
    f.write(str(build_no))
    print('Build number: {}'.format(build_no))

hf = """
#ifndef BUILD_NUMBER
  #define BUILD_NUMBER "{}"
#endif
#ifndef VERSION
  #define VERSION "{} - {}"
#endif
#ifndef VERSION_SHORT
  #define VERSION_SHORT "{}"
#endif
""".format(build_no, version+str(build_no), datetime.datetime.now(), shortversion+('0' +str(build_no))[-2:])
with open(FILENAME_VERSION_H, 'w+') as f:
    f.write(hf)