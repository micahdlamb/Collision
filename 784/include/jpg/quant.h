
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

#ifndef quant_h
#define quant_h

#include <fstream>
#include <iostream>
using namespace std;

extern int qtable[4][64];

int readqtables();

//id is 0 for Luminance, 1 for Chrominance
void writetablecore(int quality, int id);
void writeluminanceqtable(int quality);
void writebothqtables(int quality);


#endif
