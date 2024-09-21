#pragma once

#include "model/DataClass.hpp"

#include <fmt/format.h>

std::string format_currency(Data const &value) {
  if (value.is_null()) {
    return "<null>";
  }

  return fmt::format("{:>10.2F}", value.get_decimal().value_or(NAN));
}

template <typename Clock>
std::string format_timestamp(std::chrono::time_point<Clock> tp) {
  /*
  std::time_t time = secs.count();
  char timeString[std::size("yyyy-mm-dd hh:mm:ss.000")];
  std::strftime(std::data(timeString), std::size(timeString), "%F %T.000",
  std::gmtime(&time));

  return timeString;
  */

  /*
  std::chrono::time_point<std::chrono::utc_clock> epoch;

  std::cout << std::format("The time of the Unix epoch was {0:%F}T{0:%R%z}.",
  epoch) << '\n';
  */

  /*
  std::ostringstream o;

  o << fmt::format("{0:%0d}{0:%0m}{0:%0y}.", tp);

  return o.str();
  */

  return "";
}
