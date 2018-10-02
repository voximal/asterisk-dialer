
const bool doColorize = false;

/*
        Color breakout for doColorize stuff
        Color           FG      BG
        black           30      40
        red             31      41
        green           32      42
        yellow          33      43
        blue            34      44
        magenta         35      45
        cyan            36      46
        white           37      47
*/

const std::string black = "\033[30;40m";  // black on black 
const std::string red = "\033[31;40m";  // red on black
const std::string green = "\033[32;40m";  // green on black
const std::string yellow = "\033[33;40m"; // yellow on black
const std::string blue = "\033[34;40m"; // blue on black
const std::string magenta = "\033[35;40m";  // magenta on black
const std::string cyan = "\033[36;40m"; // cyan on black
const std::string white = "\033[37;40m";  // white on black
const std::string normal = "\033[0m"; // reset to system 


const std::string fg_black = "\033[30m";  // set foreground color to black
const std::string fg_red = "\033[31m";  // set foreground color to red
const std::string fg_green = "\033[32m";  // set foreground color to green
const std::string fg_yellow = "\033[33m"; // set foreground color to yellow
const std::string fg_blue = "\033[34m"; // set foreground color to blue
const std::string fg_magenta = "\033[35m";  // set foreground color to magenta (purple)
const std::string fg_cyan = "\033[36m"; // set foreground color to cyan
const std::string fg_white = "\033[37m";  // set foreground color to white
const std::string bg_black = "\033[40m";  // set background color to black
const std::string bg_red = "\033[41m";  // set background color to red
const std::string bg_green = "\033[42m";  // set background color to green
const std::string bg_yellow = "\033[43m"; // set background color to yellow
const std::string bg_blue = "\033[44m"; // set background color to blue
const std::string bg_magenta = "\033[45m";  // set background color to magenta (purple)
const std::string bg_cyan = "\033[46m"; // set background color to cyan
const std::string bg_white = "\033[47m";  // set background color to white
const std::string fg_light_grey = "\033[1;30m"; // set foreground color to light gray
const std::string fg_light_red = "\033[1;31m";  // set foreground color to light red
const std::string fg_light_green = "\033[1;32m";  // set foreground color to light green
const std::string fg_light_yellow = "\033[1;33m"; // set foreground color to light yellow
const std::string fg_light_blue = "\033[1;34m"; // set foreground color to light blue
const std::string fg_light_magenta = "\033[1;35m";  // set foreground color to light magenta (purple)
const std::string fg_light_cyan = "\033[1;36m"; // set foreground color to light cyan
const std::string fg_light_white = "\033[1;37m";  // set foreground color to light white
const std::string bg_light_grey = "\033[1;40m"; // set background color to light gray
const std::string bg_light_red = "\033[1;41m";  // set background color to light red
const std::string bg_light_green = "\033[1;42m";  // set background color to light green
const std::string bg_light_yellow = "\033[1;43m"; // set background color to light yellow
const std::string bg_light_blue = "\033[1;44m"; // set background color to light blue
const std::string bg_light_magenta = "\033[1;45m";  // set background color to light magenta (purple)
const std::string bg_light_cyan = "\033[1;46m"; // set background color to light cyan
const std::string bg_light_white = "\033[1;47m";  // set background color to light white

// other stuff, needs more setup for moving ones
const std::string xg_reset = "\033[0m"; // reset; clears all colors and styles (to white on black)
const std::string xg_bold = "\033[1m";  // bold on
const std::string xg_italics = "\033[3m"; // italics on
const std::string xg_underline = "\033[4m"; // underline on
const std::string xg_blink = "\033[5m"; // blink on
const std::string xg_reverse = "\033[7m"; // reverse video on
const std::string xg_invisible = "\033[8m"; // nondisplayed (invisible)
const std::string xg_move_yx = "\033[x;yH"; // moves cursor to line x, column y
const std::string xg_move_up = "\033[xA"; // moves cursor up x lines
const std::string xg_move_down = "\033[xB"; // moves cursor down x lines
const std::string xg_move_right = "\033[xC";  // moves cursor right x spaces
const std::string xg_move_left = "\033[xD"; // moves cursor left x spaces
const std::string xg_clear = "\033[2J"; // clear screen and home cursor
