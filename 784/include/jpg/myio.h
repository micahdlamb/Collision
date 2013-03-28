

/*             _      | |        |  _      | |      |  | |
 *            / \     | |          / \     | |  /|  |  | |             \
 *      Q___|____\  __| | | . |__Q____\  __| | (_|__|__| |  Q_|__|__|___)
 *  ___/    :      /      |___|         /               ___/          .
 *               _/                   _/
 *
 */

//  Written by Hamed Ahmadi Nejad
//    ahmadinejad@ce.sharif.edu
//    comphamed@yahoo.com

#ifndef myio_h
#define myio_h

extern int nError;

void openfile(char *s);
void closefile();

int read2bytes();
int readbyte();

void resetwritebuffer();
void writebyte(int x);
void write2bytes(int x);
void writestring(char *s);
void writetofile(char *filename);

void Error(char *s);

using namespace std;

#endif
