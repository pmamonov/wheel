typedef unsigned long time_t;

struct tm {
  unsigned char sec;
  unsigned char min;
  unsigned char hour;
  unsigned char day;
  unsigned char mon;
  unsigned year;
};

void date2stamp(struct tm *date, time_t *epoch);
void stamp2date(time_t epoch, struct tm *date);
