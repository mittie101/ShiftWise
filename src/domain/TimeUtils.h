#pragma once
#include <QString>

namespace TimeUtils {

inline QString minutesToTimeString(int minutes, bool use24h)
{
    int h = (minutes / 60) % 24;
    int m = minutes % 60;

    if (use24h) {
        return QString::asprintf("%02d:%02d", h, m);
    }

    // 12-hour with AM/PM
    static const char* const kSuffix[] = { "AM", "PM" };
    int suffix = (h >= 12) ? 1 : 0;
    int h12 = h % 12;
    if (h12 == 0) h12 = 12;   // 0 -> 12 (covers midnight and noon)
    return QString::asprintf("%d:%02d %s", h12, m, kSuffix[suffix]);
}

inline int timeStringToMinutes(const QString& s)
{
    const QString t = s.trimmed();

    // Try "HH:MM" or "H:MM" with optional AM/PM suffix (case-insensitive)
    // Accepted forms: "09:30", "9:30", "9:30 AM", "9:30 PM", "12:00 am", etc.
    const int colonPos = t.indexOf(QLatin1Char(':'));
    if (colonPos <= 0) return -1;

    bool ok = false;
    const int hours = t.left(colonPos).toInt(&ok);
    if (!ok || hours < 0) return -1;

    // After colon: up to 2 digits for minutes, then optional whitespace + AM/PM
    const QString rest = t.mid(colonPos + 1).trimmed();
    if (rest.isEmpty()) return -1;

    int minsEnd = 0;
    while (minsEnd < rest.size() && rest[minsEnd].isDigit()) ++minsEnd;
    if (minsEnd == 0) return -1;

    const int mins = rest.left(minsEnd).toInt(&ok);
    if (!ok || mins < 0 || mins > 59) return -1;

    const QString suffix = rest.mid(minsEnd).trimmed().toUpper();

    if (suffix.isEmpty()) {
        // 24-hour format
        if (hours > 23) return -1;
        return hours * 60 + mins;
    }

    // 12-hour format
    if (suffix != QLatin1String("AM") && suffix != QLatin1String("PM")) return -1;
    if (hours < 1 || hours > 12) return -1;

    int h24 = hours % 12;   // 12 AM -> 0, 12 PM -> 12
    if (suffix == QLatin1String("PM")) h24 += 12;
    return h24 * 60 + mins;
}

inline QString dayOfWeekName(int dow)
{
    static const char* const kNames[] = {
        "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
    };
    if (dow < 0 || dow > 6) return {};
    return QString::fromLatin1(kNames[dow]);
}

inline QString dayOfWeekShort(int dow)
{
    static const char* const kNames[] = {
        "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
    };
    if (dow < 0 || dow > 6) return {};
    return QString::fromLatin1(kNames[dow]);
}

} // namespace TimeUtils
