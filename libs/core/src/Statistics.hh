// Statistics.hh
//
// Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2010 Rob Caelers & Raymond Penners
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef STATISTICS_HH
#define STATISTICS_HH

#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <vector>

#include "IStatistics.hh"
#include "IInputMonitorListener.hh"
#include "Mutex.hh"

// Forward declarion of external interface.
namespace workrave
{
  class IBreak;
}

class TimePred;
class PacketBuffer;
class Core;
class IInputMonitor;

using namespace workrave;
using namespace std;

#ifdef HAVE_DISTRIBUTION
#  include "IDistributionClientMessage.hh"
#  include "PacketBuffer.hh"
#endif

class Statistics
  : public IStatistics
  , public IInputMonitorListener
#ifdef HAVE_DISTRIBUTION
  , public IDistributionClientMessage
#endif
{
private:
  enum StatsMarker
  {
    STATS_MARKER_TODAY,
    STATS_MARKER_HISTORY,
    STATS_MARKER_END,
    STATS_MARKER_STARTTIME,
    STATS_MARKER_STOPTIME,
    STATS_MARKER_BREAK_STATS,
    STATS_MARKER_MISC_STATS,
  };

  struct DailyStatsImpl : public DailyStats
  {
    //! Total time that the mouse was moving.
    gint64 total_mouse_time;

    DailyStatsImpl()
    {
      memset((void *)&start, 0, sizeof(start));
      memset((void *)&stop, 0, sizeof(stop));

      for (int i = 0; i < BREAK_ID_SIZEOF; i++)
        {
          for (int j = 0; j < STATS_BREAKVALUE_SIZEOF; j++)
            {
              break_stats[i][j] = 0;
            }
        }

      for (int j = 0; j < STATS_VALUE_SIZEOF; j++)
        {
          misc_stats[j] = 0;
        }

      // Empty marker.
      start.tm_year = 0;
      total_mouse_time = 0;
    }

    bool starts_at_date(int y, int m, int d);
    bool starts_before_date(int y, int m, int d);
    bool is_empty() const { return start.tm_year == 0; }
  };

  typedef std::vector<DailyStatsImpl *> History;
  typedef std::vector<DailyStatsImpl *>::iterator HistoryIter;
  typedef std::vector<DailyStatsImpl *>::reverse_iterator HistoryRIter;

public:
  Statistics() = default;
  ~Statistics() override;

  bool delete_all_history() override;

public:
  void init(Core *core);
  void update() override;
  void dump() override;
  void start_new_day();

  void increment_break_counter(BreakId, StatsBreakValueType st);
  void set_break_counter(BreakId bt, StatsBreakValueType st, int value);
  void add_break_counter(BreakId bt, StatsBreakValueType st, int value);

  DailyStatsImpl *get_current_day() const override;
  DailyStatsImpl *get_day(int day) const override;
  void get_day_index_by_date(int y, int m, int d, int &idx, int &next, int &prev) const override;

  int get_history_size() const override;
  void set_counter(StatsValueType t, int value);
  int64_t get_counter(StatsValueType t);

private:
  void action_notify() override;
  void mouse_notify(int x, int y, int wheel = 0) override;
  void button_notify(bool is_press) override;
  void keyboard_notify(bool repeat) override;

  bool load_current_day();
  void update_current_day(bool active);
  void load_history();

private:
  void save_day(DailyStatsImpl *stats);
  void save_day(DailyStatsImpl *stats, std::ofstream &stats_file);
  void load(std::ifstream &infile, bool history);

  void day_to_history(DailyStatsImpl *stats);
  void day_to_remote_history(DailyStatsImpl *stats);

  void add_history(DailyStatsImpl *stats);

#ifdef HAVE_DISTRIBUTION
  void init_distribution_manager();
  bool request_client_message(DistributionClientMessageID id, PacketBuffer &buffer) override;
  bool client_message(DistributionClientMessageID id, bool master, const char *client_id, PacketBuffer &buffer) override;
  bool pack_stats(PacketBuffer &buffer, const DailyStatsImpl *stats);
#endif

private:
  //! Interface to the core_control.
  Core *core{nullptr};

  //! Mouse/Keyboard monitoring.
  IInputMonitor *input_monitor{nullptr};

  //! Last time a mouse event was received.
  gint64 last_mouse_time{0};

  //! Statistics of current day.
  DailyStatsImpl *current_day{nullptr};

  //! Has the user been active on the current day?
  bool been_active{false};

  //! History
  History history;

  //! Internal locking
  Mutex lock;

  //! Previous X coordinate
  int prev_x{-1};

  //! Previous Y coordinate
  int prev_y{-1};

  //! Previous X-click coordinate
  int click_x{-1};

  //! Previous Y-click coordinate
  int click_y{-1};
};

#endif // STATISTICS_HH