/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: set ts=8 sts=2 et sw=2 tw=80:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "builtin/temporal/PlainYearMonth.h"

#include "mozilla/Assertions.h"

#include <type_traits>
#include <utility>

#include "jsnum.h"
#include "jspubtd.h"
#include "NamespaceImports.h"

#include "builtin/temporal/Calendar.h"
#include "builtin/temporal/Duration.h"
#include "builtin/temporal/PlainDate.h"
#include "builtin/temporal/Temporal.h"
#include "builtin/temporal/TemporalFields.h"
#include "builtin/temporal/TemporalParser.h"
#include "builtin/temporal/TemporalRoundingMode.h"
#include "builtin/temporal/TemporalTypes.h"
#include "builtin/temporal/TemporalUnit.h"
#include "builtin/temporal/ToString.h"
#include "ds/IdValuePair.h"
#include "gc/AllocKind.h"
#include "gc/Barrier.h"
#include "js/AllocPolicy.h"
#include "js/CallArgs.h"
#include "js/CallNonGenericMethod.h"
#include "js/Class.h"
#include "js/ErrorReport.h"
#include "js/friend/ErrorMessages.h"
#include "js/GCVector.h"
#include "js/Id.h"
#include "js/PropertyDescriptor.h"
#include "js/PropertySpec.h"
#include "js/RootingAPI.h"
#include "js/TypeDecls.h"
#include "js/Value.h"
#include "vm/BytecodeUtil.h"
#include "vm/GlobalObject.h"
#include "vm/JSAtomState.h"
#include "vm/JSContext.h"
#include "vm/JSObject.h"
#include "vm/ObjectOperations.h"
#include "vm/PlainObject.h"
#include "vm/StringType.h"

#include "vm/JSObject-inl.h"
#include "vm/NativeObject-inl.h"

using namespace js;
using namespace js::temporal;

static inline bool IsPlainYearMonth(Handle<Value> v) {
  return v.isObject() && v.toObject().is<PlainYearMonthObject>();
}

/**
 * ISOYearMonthWithinLimits ( year, month )
 */
template <typename T>
static bool ISOYearMonthWithinLimits(T year, int32_t month) {
  static_assert(std::is_same_v<T, int32_t> || std::is_same_v<T, double>);

  // Step 1.
  MOZ_ASSERT(IsInteger(year));
  MOZ_ASSERT(1 <= month && month <= 12);

  // Step 2.
  if (year < -271821 || year > 275760) {
    return false;
  }

  // Step 3.
  if (year == -271821 && month < 4) {
    return false;
  }

  // Step 4.
  if (year == 275760 && month > 9) {
    return false;
  }

  // Step 5.
  return true;
}

/**
 * ISOYearMonthWithinLimits ( year, month )
 */
bool js::temporal::ISOYearMonthWithinLimits(int32_t year, int32_t month) {
  return ::ISOYearMonthWithinLimits(year, month);
}

/**
 * CreateTemporalYearMonth ( isoYear, isoMonth, calendar, referenceISODay [ ,
 * newTarget ] )
 */
static PlainYearMonthObject* CreateTemporalYearMonth(
    JSContext* cx, const CallArgs& args, double isoYear, double isoMonth,
    double isoDay, Handle<CalendarValue> calendar) {
  MOZ_ASSERT(IsInteger(isoYear));
  MOZ_ASSERT(IsInteger(isoMonth));
  MOZ_ASSERT(IsInteger(isoDay));

  // Step 1.
  if (!ThrowIfInvalidISODate(cx, isoYear, isoMonth, isoDay)) {
    return nullptr;
  }

  // Step 2.
  if (!::ISOYearMonthWithinLimits(isoYear, int32_t(isoMonth))) {
    JS_ReportErrorNumberASCII(cx, GetErrorMessage, nullptr,
                              JSMSG_TEMPORAL_PLAIN_YEAR_MONTH_INVALID);
    return nullptr;
  }

  // Steps 3-4.
  Rooted<JSObject*> proto(cx);
  if (!GetPrototypeFromBuiltinConstructor(cx, args, JSProto_PlainYearMonth,
                                          &proto)) {
    return nullptr;
  }

  auto* obj = NewObjectWithClassProto<PlainYearMonthObject>(cx, proto);
  if (!obj) {
    return nullptr;
  }

  // Step 5.
  obj->setFixedSlot(PlainYearMonthObject::ISO_YEAR_SLOT,
                    Int32Value(int32_t(isoYear)));

  // Step 6.
  obj->setFixedSlot(PlainYearMonthObject::ISO_MONTH_SLOT,
                    Int32Value(int32_t(isoMonth)));

  // Step 7.
  obj->setFixedSlot(PlainYearMonthObject::CALENDAR_SLOT,
                    calendar.toSlotValue());

  // Step 8.
  obj->setFixedSlot(PlainYearMonthObject::ISO_DAY_SLOT,
                    Int32Value(int32_t(isoDay)));

  // Step 9.
  return obj;
}

/**
 * CreateTemporalYearMonth ( isoYear, isoMonth, calendar, referenceISODay [ ,
 * newTarget ] )
 */
static PlainYearMonthObject* CreateTemporalYearMonth(
    JSContext* cx, const PlainDate& date, Handle<CalendarValue> calendar) {
  const auto& [isoYear, isoMonth, isoDay] = date;

  // Step 1.
  if (!ThrowIfInvalidISODate(cx, date)) {
    return nullptr;
  }

  // Step 2.
  if (!::ISOYearMonthWithinLimits(isoYear, isoMonth)) {
    JS_ReportErrorNumberASCII(cx, GetErrorMessage, nullptr,
                              JSMSG_TEMPORAL_PLAIN_YEAR_MONTH_INVALID);
    return nullptr;
  }

  // Steps 3-4.
  auto* obj = NewBuiltinClassInstance<PlainYearMonthObject>(cx);
  if (!obj) {
    return nullptr;
  }

  // Step 5.
  obj->setFixedSlot(PlainYearMonthObject::ISO_YEAR_SLOT, Int32Value(isoYear));

  // Step 6.
  obj->setFixedSlot(PlainYearMonthObject::ISO_MONTH_SLOT, Int32Value(isoMonth));

  // Step 7.
  obj->setFixedSlot(PlainYearMonthObject::CALENDAR_SLOT,
                    calendar.toSlotValue());

  // Step 8.
  obj->setFixedSlot(PlainYearMonthObject::ISO_DAY_SLOT, Int32Value(isoDay));

  // Step 9.
  return obj;
}

/**
 * CreateTemporalYearMonth ( isoYear, isoMonth, calendar, referenceISODay [ ,
 * newTarget ] )
 */
PlainYearMonthObject* js::temporal::CreateTemporalYearMonth(
    JSContext* cx, Handle<PlainYearMonthWithCalendar> yearMonth) {
  MOZ_ASSERT(
      ISOYearMonthWithinLimits(yearMonth.date().year, yearMonth.date().month));
  return CreateTemporalYearMonth(cx, yearMonth, yearMonth.calendar());
}

/**
 * CreateTemporalYearMonth ( isoYear, isoMonth, calendar, referenceISODay [ ,
 * newTarget ] )
 */
bool js::temporal::CreateTemporalYearMonth(
    JSContext* cx, const PlainDate& date, Handle<CalendarValue> calendar,
    MutableHandle<PlainYearMonthWithCalendar> result) {
  const auto& [isoYear, isoMonth, isoDay] = date;

  // Step 1.
  if (!ThrowIfInvalidISODate(cx, date)) {
    return false;
  }

  // Step 2.
  if (!::ISOYearMonthWithinLimits(isoYear, isoMonth)) {
    JS_ReportErrorNumberASCII(cx, GetErrorMessage, nullptr,
                              JSMSG_TEMPORAL_PLAIN_YEAR_MONTH_INVALID);
    return false;
  }

  // Steps 3-9.
  result.set(PlainYearMonthWithCalendar{date, calendar});
  return true;
}

/**
 * ToTemporalYearMonth ( item [ , overflow ] )
 */
static bool ToTemporalYearMonth(
    JSContext* cx, Handle<JSObject*> item, TemporalOverflow overflow,
    MutableHandle<PlainYearMonthWithCalendar> result) {
  // Step 2.a.
  if (auto* plainYearMonth = item->maybeUnwrapIf<PlainYearMonthObject>()) {
    auto date = ToPlainDate(plainYearMonth);
    Rooted<CalendarValue> calendar(cx, plainYearMonth->calendar());
    if (!calendar.wrap(cx)) {
      return false;
    }

    // Step 2.a.i.
    result.set(PlainYearMonthWithCalendar{date, calendar});
    return true;
  }

  // Step 2.b.
  Rooted<CalendarValue> calendar(cx);
  if (!GetTemporalCalendarWithISODefault(cx, item, &calendar)) {
    return false;
  }

  // Step 2.c.
  Rooted<TemporalFields> fields(cx);
  if (!PrepareCalendarFields(cx, calendar, item,
                             {
                                 CalendarField::Month,
                                 CalendarField::MonthCode,
                                 CalendarField::Year,
                             },
                             &fields)) {
    return false;
  }

  // Step 2.d.
  return CalendarYearMonthFromFields(cx, calendar, fields, overflow, result);
}

/**
 * ToTemporalYearMonth ( item [ , overflow ] )
 */
static bool ToTemporalYearMonth(
    JSContext* cx, Handle<Value> item, TemporalOverflow overflow,
    MutableHandle<PlainYearMonthWithCalendar> result) {
  // Step 1. (Not applicable in our implementation.)

  // Step 2.
  if (item.isObject()) {
    Rooted<JSObject*> itemObj(cx, &item.toObject());
    return ToTemporalYearMonth(cx, itemObj, overflow, result);
  }

  // Step 3.
  if (!item.isString()) {
    ReportValueError(cx, JSMSG_UNEXPECTED_TYPE, JSDVG_IGNORE_STACK, item,
                     nullptr, "not a string");
    return false;
  }
  Rooted<JSString*> string(cx, item.toString());

  // Step 4.
  PlainDate date;
  Rooted<JSString*> calendarString(cx);
  if (!ParseTemporalYearMonthString(cx, string, &date, &calendarString)) {
    return false;
  }

  // Steps 5-8.
  Rooted<CalendarValue> calendar(cx, CalendarValue(CalendarId::ISO8601));
  if (calendarString) {
    if (!ToBuiltinCalendar(cx, calendarString, &calendar)) {
      return false;
    }
  }

  // Step 9.
  Rooted<PlainYearMonthObject*> obj(
      cx, CreateTemporalYearMonth(cx, date, calendar));
  if (!obj) {
    return false;
  }

  // FIXME: spec issue - |obj| should be unobservable.

  // Steps 10-11.
  return CalendarYearMonthFromFields(cx, calendar, obj,
                                     TemporalOverflow::Constrain, result);
}

/**
 * ToTemporalYearMonth ( item [ , overflow ] )
 */
static bool ToTemporalYearMonth(
    JSContext* cx, Handle<Value> item,
    MutableHandle<PlainYearMonthWithCalendar> result) {
  return ToTemporalYearMonth(cx, item, TemporalOverflow::Constrain, result);
}

/**
 * DifferenceTemporalPlainYearMonth ( operation, yearMonth, other, options )
 */
static bool DifferenceTemporalPlainYearMonth(JSContext* cx,
                                             TemporalDifference operation,
                                             const CallArgs& args) {
  Rooted<PlainYearMonthObject*> yearMonth(
      cx, &args.thisv().toObject().as<PlainYearMonthObject>());

  // Step 1. (Not applicable in our implementation.)

  // Step 2.
  Rooted<PlainYearMonthWithCalendar> other(cx);
  if (!ToTemporalYearMonth(cx, args.get(0), &other)) {
    return false;
  }

  Rooted<JSObject*> otherYearMonth(cx);
  if (args.get(0).isObject() &&
      args[0].toObject().canUnwrapAs<PlainYearMonthObject>()) {
    // FIXME: spec issue - this special casing shouldn't be needed.
    otherYearMonth = &args[0].toObject();
  } else {
    auto* obj = CreateTemporalYearMonth(cx, other);
    if (!obj) {
      return false;
    }
    otherYearMonth = obj;
  }

  // Step 3.
  Rooted<CalendarValue> calendar(cx, yearMonth->calendar());

  // Step 4.
  if (!CalendarEquals(calendar, other.calendar())) {
    JS_ReportErrorNumberASCII(
        cx, GetErrorMessage, nullptr, JSMSG_TEMPORAL_CALENDAR_INCOMPATIBLE,
        ToTemporalCalendarIdentifier(calendar).data(),
        ToTemporalCalendarIdentifier(other.calendar()).data());
    return false;
  }

  // Steps 5-6.
  DifferenceSettings settings;
  Rooted<PlainObject*> resolvedOptions(cx);
  if (args.hasDefined(1)) {
    Rooted<JSObject*> options(
        cx, RequireObjectArg(cx, "options", ToName(operation), args[1]));
    if (!options) {
      return false;
    }

    // Step 6.
    if (!GetDifferenceSettings(cx, operation, options, TemporalUnitGroup::Date,
                               TemporalUnit::Month, TemporalUnit::Month,
                               TemporalUnit::Year, &settings)) {
      return false;
    }
  } else {
    // Steps 5-6.
    settings = {
        TemporalUnit::Month,
        TemporalUnit::Year,
        TemporalRoundingMode::Trunc,
        Increment{1},
    };
  }

  // Step 7.
  if (ToPlainDate(yearMonth) == other.date()) {
    auto* obj = CreateTemporalDuration(cx, {});
    if (!obj) {
      return false;
    }

    args.rval().setObject(*obj);
    return true;
  }

  // Step 8.
  Rooted<TemporalFields> thisFields(cx);
  if (!PrepareCalendarFieldsAndFieldNames(cx, calendar, yearMonth,
                                          {
                                              CalendarField::MonthCode,
                                              CalendarField::Year,
                                          },
                                          &thisFields)) {
    return false;
  }

  // Remember field names of unmodified |thisFields|.
  auto thisFieldNames = thisFields.keys();

  // Step 9.
  MOZ_ASSERT(!thisFields.has(TemporalField::Day));
  thisFields.setDay(1);

  // Step 10.
  Rooted<PlainDateWithCalendar> thisDate(cx);
  if (!CalendarDateFromFields(cx, calendar, thisFields,
                              TemporalOverflow::Constrain, &thisDate)) {
    return false;
  }

  // Step 11.
  Rooted<TemporalFields> otherFields(cx);
  if (!PrepareTemporalFields(cx, otherYearMonth, thisFieldNames,
                             &otherFields)) {
    return false;
  }

  // Step 12.
  MOZ_ASSERT(!otherFields.has(TemporalField::Day));
  otherFields.setDay(1);

  // Step 13.
  Rooted<PlainDateWithCalendar> otherDate(cx);
  if (!CalendarDateFromFields(cx, calendar, otherFields,
                              TemporalOverflow::Constrain, &otherDate)) {
    return false;
  }

  // Step 14.
  DateDuration until;
  if (!CalendarDateUntil(cx, calendar, thisDate, otherDate,
                         settings.largestUnit, &until)) {
    return false;
  }

  // We only care about years and months here, all other fields are set to zero.
  auto dateDuration = DateDuration{until.years, until.months};

  // Step 15. (Moved below)

  // Step 16.
  if (settings.smallestUnit != TemporalUnit::Month ||
      settings.roundingIncrement != Increment{1}) {
    // Step 15. (Reordered)
    auto duration = NormalizedDuration{dateDuration, {}};

    // Step 16.a.
    auto otherDateTime = PlainDateTime{otherDate, {}};
    auto destEpochNs = GetUTCEpochNanoseconds(otherDateTime);

    // Step 16.b.
    auto dateTime = PlainDateTime{thisDate, {}};

    // Step 16.c
    Rooted<TimeZoneValue> timeZone(cx, TimeZoneValue{});
    RoundedRelativeDuration relative;
    if (!RoundRelativeDuration(
            cx, duration, destEpochNs, dateTime, calendar, timeZone,
            settings.largestUnit, settings.roundingIncrement,
            settings.smallestUnit, settings.roundingMode, &relative)) {
      return false;
    }
    MOZ_ASSERT(IsValidDuration(relative.duration));

    dateDuration = relative.duration.toDateDuration();
  }

  // Step 17.
  auto duration =
      Duration{double(dateDuration.years), double(dateDuration.months)};
  if (operation == TemporalDifference::Since) {
    duration = duration.negate();
  }

  auto* obj = CreateTemporalDuration(cx, duration);
  if (!obj) {
    return false;
  }

  args.rval().setObject(*obj);
  return true;
}

enum class PlainYearMonthDuration { Add, Subtract };

/**
 * AddDurationToOrSubtractDurationFromPlainYearMonth ( operation, yearMonth,
 * temporalDurationLike, options )
 */
static bool AddDurationToOrSubtractDurationFromPlainYearMonth(
    JSContext* cx, PlainYearMonthDuration operation, const CallArgs& args) {
  Rooted<PlainYearMonthObject*> yearMonth(
      cx, &args.thisv().toObject().as<PlainYearMonthObject>());

  // Step 1.
  Duration duration;
  if (!ToTemporalDurationRecord(cx, args.get(0), &duration)) {
    return false;
  }

  // Step 2.
  if (operation == PlainYearMonthDuration::Subtract) {
    duration = duration.negate();
  }

  // Steps 3-4.
  auto overflow = TemporalOverflow::Constrain;
  if (args.hasDefined(1)) {
    // Step 3.
    const char* name =
        operation == PlainYearMonthDuration::Add ? "add" : "subtract";
    Rooted<JSObject*> options(cx,
                              RequireObjectArg(cx, "options", name, args[1]));
    if (!options) {
      return false;
    }

    // Step 4.
    if (!GetTemporalOverflowOption(cx, options, &overflow)) {
      return false;
    }
  }

  // Step 5.
  auto timeDuration = NormalizeTimeDuration(duration);

  // Step 6.
  auto balancedTime = BalanceTimeDuration(timeDuration, TemporalUnit::Day);

  // Steps 7 and 16. (Reordered)
  auto durationToAdd = DateDuration{
      int64_t(duration.years),
      int64_t(duration.months),
      int64_t(duration.weeks),
      int64_t(duration.days) + balancedTime.days,
  };

  // Step 8.
  int32_t sign = DurationSign(durationToAdd);

  // Step 9.
  Rooted<CalendarValue> calendar(cx, yearMonth->calendar());

  // Step 10.
  Rooted<TemporalFields> fields(cx);
  if (!PrepareCalendarFieldsAndFieldNames(cx, calendar, yearMonth,
                                          {
                                              CalendarField::MonthCode,
                                              CalendarField::Year,
                                          },
                                          &fields)) {
    return false;
  }

  // Remember field names of unmodified |fields|.
  auto fieldNames = fields.keys();

  // Step 11.
  Rooted<TemporalFields> fieldsCopy(cx, TemporalFields{fields});

  // Step 12.
  MOZ_ASSERT(!fields.has(TemporalField::Day));
  fields.setDay(1);

  // Step 13.
  Rooted<PlainDateWithCalendar> intermediateDate(cx);
  if (!CalendarDateFromFields(cx, calendar, fields, TemporalOverflow::Constrain,
                              &intermediateDate)) {
    return false;
  }

  // Steps 14-15.
  Rooted<PlainDateWithCalendar> date(cx);
  if (sign < 0) {
    // |intermediateDate| is initialized to the first day of |yearMonth|'s
    // month. Compute the last day of |yearMonth|'s month by first adding one
    // month and then subtracting one day.
    //
    // This is roughly equivalent to these calls:
    //
    // js> var ym = new Temporal.PlainYearMonth(2023, 1);
    // js> ym.toPlainDate({day: 1}).add({months: 1}).subtract({days: 1}).day
    // 31
    //
    // For many calendars this is equivalent to `ym.daysInMonth`, except when
    // some days are skipped, for example consider the Julian-to-Gregorian
    // calendar transition.

    // Step 14.a.
    auto oneMonthDuration = DateDuration{0, 1};

    // Step 14.b.
    PlainDate nextMonth;
    if (!CalendarDateAdd(cx, calendar, intermediateDate, oneMonthDuration,
                         TemporalOverflow::Constrain, &nextMonth)) {
      return false;
    }

    // Step 14.c.
    auto endOfMonthISO =
        BalanceISODate(nextMonth.year, nextMonth.month, nextMonth.day - 1);

    // Step 14.d.
    Rooted<PlainDateWithCalendar> endOfMonth(cx);
    if (!CreateTemporalDate(cx, endOfMonthISO, calendar, &endOfMonth)) {
      return false;
    }

    // Step 14.e.
    Rooted<Value> day(cx);
    if (!CalendarDay(cx, calendar, endOfMonth.date(), &day)) {
      return false;
    }
    MOZ_ASSERT(day.isInt32());

    // Step 14.f.
    MOZ_ASSERT(!fieldsCopy.has(TemporalField::Day));
    fieldsCopy.setDay(day.toInt32());

    // Step 14.g.
    if (!CalendarDateFromFields(cx, calendar, fieldsCopy,
                                TemporalOverflow::Constrain, &date)) {
      return false;
    }
  } else {
    // Step 15.a.
    date = intermediateDate;
  }

  // Step 16. (Moved above)

  // Step 17.
  PlainDate addedDate;
  if (!AddDate(cx, calendar, date, durationToAdd, overflow, &addedDate)) {
    return false;
  }

  // FIXME: spec issue - addedDateObj should be unobservable.

  Rooted<PlainDateObject*> addedDateObj(
      cx, CreateTemporalDate(cx, addedDate, calendar));
  if (!addedDateObj) {
    return false;
  }

  // Step 18.
  Rooted<TemporalFields> addedDateFields(cx);
  if (!PrepareTemporalFields(cx, addedDateObj, fieldNames, &addedDateFields)) {
    return false;
  }

  // Step 19.
  Rooted<PlainYearMonthWithCalendar> result(cx);
  if (!CalendarYearMonthFromFields(cx, calendar, addedDateFields, overflow,
                                   &result)) {
    return false;
  }

  auto* obj = CreateTemporalYearMonth(cx, result);
  if (!obj) {
    return false;
  }

  args.rval().setObject(*obj);
  return true;
}

/**
 * Temporal.PlainYearMonth ( isoYear, isoMonth [ , calendarLike [ ,
 * referenceISODay ] ] )
 */
static bool PlainYearMonthConstructor(JSContext* cx, unsigned argc, Value* vp) {
  CallArgs args = CallArgsFromVp(argc, vp);

  // Step 1.
  if (!ThrowIfNotConstructing(cx, args, "Temporal.PlainYearMonth")) {
    return false;
  }

  // Step 3.
  double isoYear;
  if (!ToIntegerWithTruncation(cx, args.get(0), "year", &isoYear)) {
    return false;
  }

  // Step 4.
  double isoMonth;
  if (!ToIntegerWithTruncation(cx, args.get(1), "month", &isoMonth)) {
    return false;
  }

  // Steps 5-8.
  Rooted<CalendarValue> calendar(cx, CalendarValue(CalendarId::ISO8601));
  if (args.hasDefined(2)) {
    // Step 6.
    if (!args[2].isString()) {
      ReportValueError(cx, JSMSG_UNEXPECTED_TYPE, JSDVG_IGNORE_STACK, args[2],
                       nullptr, "not a string");
      return false;
    }

    // Steps 7-8.
    Rooted<JSString*> calendarString(cx, args[2].toString());
    if (!ToBuiltinCalendar(cx, calendarString, &calendar)) {
      return false;
    }
  }

  // Steps 2 and 9.
  double isoDay = 1;
  if (args.hasDefined(3)) {
    if (!ToIntegerWithTruncation(cx, args[3], "day", &isoDay)) {
      return false;
    }
  }

  // Step 10.
  auto* yearMonth =
      CreateTemporalYearMonth(cx, args, isoYear, isoMonth, isoDay, calendar);
  if (!yearMonth) {
    return false;
  }

  args.rval().setObject(*yearMonth);
  return true;
}

/**
 * Temporal.PlainYearMonth.from ( item [ , options ] )
 */
static bool PlainYearMonth_from(JSContext* cx, unsigned argc, Value* vp) {
  CallArgs args = CallArgsFromVp(argc, vp);

  // Steps 1-2.
  auto overflow = TemporalOverflow::Constrain;
  if (args.hasDefined(1)) {
    // Step 1.
    Rooted<JSObject*> options(cx,
                              RequireObjectArg(cx, "options", "from", args[1]));
    if (!options) {
      return false;
    }

    // Step 2.
    if (!GetTemporalOverflowOption(cx, options, &overflow)) {
      return false;
    }
  }

  // Step 3.
  Rooted<PlainYearMonthWithCalendar> yearMonth(cx);
  if (!ToTemporalYearMonth(cx, args.get(0), overflow, &yearMonth)) {
    return false;
  }

  auto* result = CreateTemporalYearMonth(cx, yearMonth);
  if (!result) {
    return false;
  }

  args.rval().setObject(*result);
  return true;
}

/**
 * Temporal.PlainYearMonth.compare ( one, two )
 */
static bool PlainYearMonth_compare(JSContext* cx, unsigned argc, Value* vp) {
  CallArgs args = CallArgsFromVp(argc, vp);

  // Step 1.
  Rooted<PlainYearMonthWithCalendar> one(cx);
  if (!ToTemporalYearMonth(cx, args.get(0), &one)) {
    return false;
  }

  // Step 2.
  Rooted<PlainYearMonthWithCalendar> two(cx);
  if (!ToTemporalYearMonth(cx, args.get(1), &two)) {
    return false;
  }

  // Step 3.
  args.rval().setInt32(CompareISODate(one, two));
  return true;
}

/**
 * get Temporal.PlainYearMonth.prototype.calendarId
 */
static bool PlainYearMonth_calendarId(JSContext* cx, const CallArgs& args) {
  auto* yearMonth = &args.thisv().toObject().as<PlainYearMonthObject>();
  Rooted<CalendarValue> calendar(cx, yearMonth->calendar());

  // Step 3.
  auto* calendarId = ToTemporalCalendarIdentifier(cx, calendar);
  if (!calendarId) {
    return false;
  }

  args.rval().setString(calendarId);
  return true;
}

/**
 * get Temporal.PlainYearMonth.prototype.calendarId
 */
static bool PlainYearMonth_calendarId(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_calendarId>(
      cx, args);
}

/**
 * get Temporal.PlainYearMonth.prototype.era
 */
static bool PlainYearMonth_era(JSContext* cx, const CallArgs& args) {
  auto* yearMonth = &args.thisv().toObject().as<PlainYearMonthObject>();
  Rooted<CalendarValue> calendar(cx, yearMonth->calendar());

  // Step 3.
  return CalendarEra(cx, calendar, ToPlainDate(yearMonth), args.rval());
}

/**
 * get Temporal.PlainYearMonth.prototype.era
 */
static bool PlainYearMonth_era(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_era>(cx, args);
}

/**
 * get Temporal.PlainYearMonth.prototype.eraYear
 */
static bool PlainYearMonth_eraYear(JSContext* cx, const CallArgs& args) {
  auto* yearMonth = &args.thisv().toObject().as<PlainYearMonthObject>();
  Rooted<CalendarValue> calendar(cx, yearMonth->calendar());

  // Steps 3-5.
  return CalendarEraYear(cx, calendar, ToPlainDate(yearMonth), args.rval());
}

/**
 * get Temporal.PlainYearMonth.prototype.eraYear
 */
static bool PlainYearMonth_eraYear(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_eraYear>(cx,
                                                                        args);
}

/**
 * get Temporal.PlainYearMonth.prototype.year
 */
static bool PlainYearMonth_year(JSContext* cx, const CallArgs& args) {
  auto* yearMonth = &args.thisv().toObject().as<PlainYearMonthObject>();
  Rooted<CalendarValue> calendar(cx, yearMonth->calendar());

  // Step 3.
  return CalendarYear(cx, calendar, ToPlainDate(yearMonth), args.rval());
}

/**
 * get Temporal.PlainYearMonth.prototype.year
 */
static bool PlainYearMonth_year(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_year>(cx, args);
}

/**
 * get Temporal.PlainYearMonth.prototype.month
 */
static bool PlainYearMonth_month(JSContext* cx, const CallArgs& args) {
  auto* yearMonth = &args.thisv().toObject().as<PlainYearMonthObject>();
  Rooted<CalendarValue> calendar(cx, yearMonth->calendar());

  // Step 3.
  return CalendarMonth(cx, calendar, ToPlainDate(yearMonth), args.rval());
}

/**
 * get Temporal.PlainYearMonth.prototype.month
 */
static bool PlainYearMonth_month(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_month>(cx, args);
}

/**
 * get Temporal.PlainYearMonth.prototype.monthCode
 */
static bool PlainYearMonth_monthCode(JSContext* cx, const CallArgs& args) {
  auto* yearMonth = &args.thisv().toObject().as<PlainYearMonthObject>();
  Rooted<CalendarValue> calendar(cx, yearMonth->calendar());

  // Step 3.
  return CalendarMonthCode(cx, calendar, ToPlainDate(yearMonth), args.rval());
}

/**
 * get Temporal.PlainYearMonth.prototype.monthCode
 */
static bool PlainYearMonth_monthCode(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_monthCode>(cx,
                                                                          args);
}

/**
 * get Temporal.PlainYearMonth.prototype.daysInYear
 */
static bool PlainYearMonth_daysInYear(JSContext* cx, const CallArgs& args) {
  auto* yearMonth = &args.thisv().toObject().as<PlainYearMonthObject>();
  Rooted<CalendarValue> calendar(cx, yearMonth->calendar());

  // Step 3.
  return CalendarDaysInYear(cx, calendar, ToPlainDate(yearMonth), args.rval());
}

/**
 * get Temporal.PlainYearMonth.prototype.daysInYear
 */
static bool PlainYearMonth_daysInYear(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_daysInYear>(
      cx, args);
}

/**
 * get Temporal.PlainYearMonth.prototype.daysInMonth
 */
static bool PlainYearMonth_daysInMonth(JSContext* cx, const CallArgs& args) {
  auto* yearMonth = &args.thisv().toObject().as<PlainYearMonthObject>();
  Rooted<CalendarValue> calendar(cx, yearMonth->calendar());

  // Step 3.
  return CalendarDaysInMonth(cx, calendar, ToPlainDate(yearMonth), args.rval());
}

/**
 * get Temporal.PlainYearMonth.prototype.daysInMonth
 */
static bool PlainYearMonth_daysInMonth(JSContext* cx, unsigned argc,
                                       Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_daysInMonth>(
      cx, args);
}

/**
 * get Temporal.PlainYearMonth.prototype.monthsInYear
 */
static bool PlainYearMonth_monthsInYear(JSContext* cx, const CallArgs& args) {
  auto* yearMonth = &args.thisv().toObject().as<PlainYearMonthObject>();
  Rooted<CalendarValue> calendar(cx, yearMonth->calendar());

  // Step 3.
  return CalendarMonthsInYear(cx, calendar, ToPlainDate(yearMonth),
                              args.rval());
}

/**
 * get Temporal.PlainYearMonth.prototype.monthsInYear
 */
static bool PlainYearMonth_monthsInYear(JSContext* cx, unsigned argc,
                                        Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_monthsInYear>(
      cx, args);
}

/**
 * get Temporal.PlainYearMonth.prototype.inLeapYear
 */
static bool PlainYearMonth_inLeapYear(JSContext* cx, const CallArgs& args) {
  auto* yearMonth = &args.thisv().toObject().as<PlainYearMonthObject>();
  Rooted<CalendarValue> calendar(cx, yearMonth->calendar());

  // Step 3.
  return CalendarInLeapYear(cx, calendar, ToPlainDate(yearMonth), args.rval());
}

/**
 * get Temporal.PlainYearMonth.prototype.inLeapYear
 */
static bool PlainYearMonth_inLeapYear(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_inLeapYear>(
      cx, args);
}

/**
 * Temporal.PlainYearMonth.prototype.with ( temporalYearMonthLike [ , options ]
 * )
 */
static bool PlainYearMonth_with(JSContext* cx, const CallArgs& args) {
  Rooted<PlainYearMonthObject*> yearMonth(
      cx, &args.thisv().toObject().as<PlainYearMonthObject>());

  // Step 3.
  Rooted<JSObject*> temporalYearMonthLike(
      cx, RequireObjectArg(cx, "temporalYearMonthLike", "with", args.get(0)));
  if (!temporalYearMonthLike) {
    return false;
  }
  if (!ThrowIfTemporalLikeObject(cx, temporalYearMonthLike)) {
    return false;
  }

  // Steps 4-5.
  auto overflow = TemporalOverflow::Constrain;
  if (args.hasDefined(1)) {
    // Step 4.
    Rooted<JSObject*> options(cx,
                              RequireObjectArg(cx, "options", "with", args[1]));
    if (!options) {
      return false;
    }

    // Step 5.
    if (!GetTemporalOverflowOption(cx, options, &overflow)) {
      return false;
    }
  }

  // Step 6.
  Rooted<CalendarValue> calendar(cx, yearMonth->calendar());

  // Step 7.
  Rooted<TemporalFields> fields(cx);
  if (!PrepareCalendarFieldsAndFieldNames(cx, calendar, yearMonth,
                                          {
                                              CalendarField::Month,
                                              CalendarField::MonthCode,
                                              CalendarField::Year,
                                          },
                                          &fields)) {
    return false;
  }

  // Step 8.
  Rooted<TemporalFields> partialYearMonth(cx);
  if (!PreparePartialTemporalFields(cx, temporalYearMonthLike, fields.keys(),
                                    &partialYearMonth)) {
    return false;
  }
  MOZ_ASSERT(!partialYearMonth.keys().isEmpty());

  // Step 9.
  Rooted<TemporalFields> mergedFields(
      cx, CalendarMergeFields(calendar, fields, partialYearMonth));

  // Step 10.
  if (!PrepareTemporalFields(cx, mergedFields, fields.keys(), &fields)) {
    return false;
  }

  // Step 10.
  Rooted<PlainYearMonthWithCalendar> result(cx);
  if (!CalendarYearMonthFromFields(cx, calendar, fields, overflow, &result)) {
    return false;
  }

  auto* obj = CreateTemporalYearMonth(cx, result);
  if (!obj) {
    return false;
  }

  args.rval().setObject(*obj);
  return true;
}

/**
 * Temporal.PlainYearMonth.prototype.with ( temporalYearMonthLike [ , options ]
 * )
 */
static bool PlainYearMonth_with(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_with>(cx, args);
}

/**
 * Temporal.PlainYearMonth.prototype.add ( temporalDurationLike [ , options ] )
 */
static bool PlainYearMonth_add(JSContext* cx, const CallArgs& args) {
  // Step 3.
  return AddDurationToOrSubtractDurationFromPlainYearMonth(
      cx, PlainYearMonthDuration::Add, args);
}

/**
 * Temporal.PlainYearMonth.prototype.add ( temporalDurationLike [ , options ] )
 */
static bool PlainYearMonth_add(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_add>(cx, args);
}

/**
 * Temporal.PlainYearMonth.prototype.subtract ( temporalDurationLike [ , options
 * ] )
 */
static bool PlainYearMonth_subtract(JSContext* cx, const CallArgs& args) {
  // Step 3.
  return AddDurationToOrSubtractDurationFromPlainYearMonth(
      cx, PlainYearMonthDuration::Subtract, args);
}

/**
 * Temporal.PlainYearMonth.prototype.subtract ( temporalDurationLike [ , options
 * ] )
 */
static bool PlainYearMonth_subtract(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_subtract>(cx,
                                                                         args);
}

/**
 * Temporal.PlainYearMonth.prototype.until ( other [ , options ] )
 */
static bool PlainYearMonth_until(JSContext* cx, const CallArgs& args) {
  // Step 3.
  return DifferenceTemporalPlainYearMonth(cx, TemporalDifference::Until, args);
}

/**
 * Temporal.PlainYearMonth.prototype.until ( other [ , options ] )
 */
static bool PlainYearMonth_until(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_until>(cx, args);
}

/**
 * Temporal.PlainYearMonth.prototype.since ( other [ , options ] )
 */
static bool PlainYearMonth_since(JSContext* cx, const CallArgs& args) {
  // Step 3.
  return DifferenceTemporalPlainYearMonth(cx, TemporalDifference::Since, args);
}

/**
 * Temporal.PlainYearMonth.prototype.since ( other [ , options ] )
 */
static bool PlainYearMonth_since(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_since>(cx, args);
}

/**
 * Temporal.PlainYearMonth.prototype.equals ( other )
 */
static bool PlainYearMonth_equals(JSContext* cx, const CallArgs& args) {
  auto* yearMonth = &args.thisv().toObject().as<PlainYearMonthObject>();
  auto date = ToPlainDate(yearMonth);
  Rooted<CalendarValue> calendar(cx, yearMonth->calendar());

  // Step 3.
  Rooted<PlainYearMonthWithCalendar> other(cx);
  if (!ToTemporalYearMonth(cx, args.get(0), &other)) {
    return false;
  }

  // Steps 4-7.
  bool equals =
      date == other.date() && CalendarEquals(calendar, other.calendar());

  args.rval().setBoolean(equals);
  return true;
}

/**
 * Temporal.PlainYearMonth.prototype.equals ( other )
 */
static bool PlainYearMonth_equals(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_equals>(cx,
                                                                       args);
}

/**
 * Temporal.PlainYearMonth.prototype.toString ( [ options ] )
 */
static bool PlainYearMonth_toString(JSContext* cx, const CallArgs& args) {
  Rooted<PlainYearMonthObject*> yearMonth(
      cx, &args.thisv().toObject().as<PlainYearMonthObject>());

  auto showCalendar = ShowCalendar::Auto;
  if (args.hasDefined(0)) {
    // Step 3.
    Rooted<JSObject*> options(
        cx, RequireObjectArg(cx, "options", "toString", args[0]));
    if (!options) {
      return false;
    }

    // Step 4.
    if (!GetTemporalShowCalendarNameOption(cx, options, &showCalendar)) {
      return false;
    }
  }

  // Step 5.
  JSString* str = TemporalYearMonthToString(cx, yearMonth, showCalendar);
  if (!str) {
    return false;
  }

  args.rval().setString(str);
  return true;
}

/**
 * Temporal.PlainYearMonth.prototype.toString ( [ options ] )
 */
static bool PlainYearMonth_toString(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_toString>(cx,
                                                                         args);
}

/**
 * Temporal.PlainYearMonth.prototype.toLocaleString ( [ locales [ , options ] ]
 * )
 */
static bool PlainYearMonth_toLocaleString(JSContext* cx, const CallArgs& args) {
  Rooted<PlainYearMonthObject*> yearMonth(
      cx, &args.thisv().toObject().as<PlainYearMonthObject>());

  // Step 3.
  JSString* str = TemporalYearMonthToString(cx, yearMonth, ShowCalendar::Auto);
  if (!str) {
    return false;
  }

  args.rval().setString(str);
  return true;
}

/**
 * Temporal.PlainYearMonth.prototype.toLocaleString ( [ locales [ , options ] ]
 * )
 */
static bool PlainYearMonth_toLocaleString(JSContext* cx, unsigned argc,
                                          Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_toLocaleString>(
      cx, args);
}

/**
 * Temporal.PlainYearMonth.prototype.toJSON ( )
 */
static bool PlainYearMonth_toJSON(JSContext* cx, const CallArgs& args) {
  Rooted<PlainYearMonthObject*> yearMonth(
      cx, &args.thisv().toObject().as<PlainYearMonthObject>());

  // Step 3.
  JSString* str = TemporalYearMonthToString(cx, yearMonth, ShowCalendar::Auto);
  if (!str) {
    return false;
  }

  args.rval().setString(str);
  return true;
}

/**
 * Temporal.PlainYearMonth.prototype.toJSON ( )
 */
static bool PlainYearMonth_toJSON(JSContext* cx, unsigned argc, Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_toJSON>(cx,
                                                                       args);
}

/**
 *  Temporal.PlainYearMonth.prototype.valueOf ( )
 */
static bool PlainYearMonth_valueOf(JSContext* cx, unsigned argc, Value* vp) {
  JS_ReportErrorNumberASCII(cx, GetErrorMessage, nullptr, JSMSG_CANT_CONVERT_TO,
                            "PlainYearMonth", "primitive type");
  return false;
}

/**
 * Temporal.PlainYearMonth.prototype.toPlainDate ( item )
 */
static bool PlainYearMonth_toPlainDate(JSContext* cx, const CallArgs& args) {
  Rooted<PlainYearMonthObject*> yearMonth(
      cx, &args.thisv().toObject().as<PlainYearMonthObject>());

  // Step 3.
  Rooted<JSObject*> item(
      cx, RequireObjectArg(cx, "item", "toPlainDate", args.get(0)));
  if (!item) {
    return false;
  }

  // Step 4.
  Rooted<CalendarValue> calendar(cx, yearMonth->calendar());

  // Step 5.
  Rooted<TemporalFields> receiverFields(cx);
  if (!PrepareCalendarFieldsAndFieldNames(cx, calendar, yearMonth,
                                          {
                                              CalendarField::MonthCode,
                                              CalendarField::Year,
                                          },
                                          &receiverFields)) {
    return false;
  }

  // Step 6.
  Rooted<TemporalFields> inputFields(cx);
  if (!PrepareCalendarFieldsAndFieldNames(cx, calendar, item,
                                          {
                                              CalendarField::Day,
                                          },
                                          &inputFields)) {
    return false;
  }

  // Step 7.
  Rooted<TemporalFields> mergedFields(
      cx, CalendarMergeFields(calendar, receiverFields, inputFields));

  // Step 8.
  auto concatenatedFieldNames = receiverFields.keys() + inputFields.keys();

  // Step 9.
  if (!PrepareTemporalFields(cx, mergedFields, concatenatedFieldNames,
                             &mergedFields)) {
    return false;
  }

  // Step 10.
  Rooted<PlainDateWithCalendar> result(cx);
  if (!CalendarDateFromFields(cx, calendar, mergedFields,
                              TemporalOverflow::Constrain, &result)) {
    return false;
  }

  auto* obj = CreateTemporalDate(cx, result);
  if (!obj) {
    return false;
  }

  args.rval().setObject(*obj);
  return true;
}

/**
 * Temporal.PlainYearMonth.prototype.toPlainDate ( item )
 */
static bool PlainYearMonth_toPlainDate(JSContext* cx, unsigned argc,
                                       Value* vp) {
  // Steps 1-2.
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod<IsPlainYearMonth, PlainYearMonth_toPlainDate>(
      cx, args);
}

const JSClass PlainYearMonthObject::class_ = {
    "Temporal.PlainYearMonth",
    JSCLASS_HAS_RESERVED_SLOTS(PlainYearMonthObject::SLOT_COUNT) |
        JSCLASS_HAS_CACHED_PROTO(JSProto_PlainYearMonth),
    JS_NULL_CLASS_OPS,
    &PlainYearMonthObject::classSpec_,
};

const JSClass& PlainYearMonthObject::protoClass_ = PlainObject::class_;

static const JSFunctionSpec PlainYearMonth_methods[] = {
    JS_FN("from", PlainYearMonth_from, 1, 0),
    JS_FN("compare", PlainYearMonth_compare, 2, 0),
    JS_FS_END,
};

static const JSFunctionSpec PlainYearMonth_prototype_methods[] = {
    JS_FN("with", PlainYearMonth_with, 1, 0),
    JS_FN("add", PlainYearMonth_add, 1, 0),
    JS_FN("subtract", PlainYearMonth_subtract, 1, 0),
    JS_FN("until", PlainYearMonth_until, 1, 0),
    JS_FN("since", PlainYearMonth_since, 1, 0),
    JS_FN("equals", PlainYearMonth_equals, 1, 0),
    JS_FN("toString", PlainYearMonth_toString, 0, 0),
    JS_FN("toLocaleString", PlainYearMonth_toLocaleString, 0, 0),
    JS_FN("toJSON", PlainYearMonth_toJSON, 0, 0),
    JS_FN("valueOf", PlainYearMonth_valueOf, 0, 0),
    JS_FN("toPlainDate", PlainYearMonth_toPlainDate, 1, 0),
    JS_FS_END,
};

static const JSPropertySpec PlainYearMonth_prototype_properties[] = {
    JS_PSG("calendarId", PlainYearMonth_calendarId, 0),
    JS_PSG("era", PlainYearMonth_era, 0),
    JS_PSG("eraYear", PlainYearMonth_eraYear, 0),
    JS_PSG("year", PlainYearMonth_year, 0),
    JS_PSG("month", PlainYearMonth_month, 0),
    JS_PSG("monthCode", PlainYearMonth_monthCode, 0),
    JS_PSG("daysInYear", PlainYearMonth_daysInYear, 0),
    JS_PSG("daysInMonth", PlainYearMonth_daysInMonth, 0),
    JS_PSG("monthsInYear", PlainYearMonth_monthsInYear, 0),
    JS_PSG("inLeapYear", PlainYearMonth_inLeapYear, 0),
    JS_STRING_SYM_PS(toStringTag, "Temporal.PlainYearMonth", JSPROP_READONLY),
    JS_PS_END,
};

const ClassSpec PlainYearMonthObject::classSpec_ = {
    GenericCreateConstructor<PlainYearMonthConstructor, 2,
                             gc::AllocKind::FUNCTION>,
    GenericCreatePrototype<PlainYearMonthObject>,
    PlainYearMonth_methods,
    nullptr,
    PlainYearMonth_prototype_methods,
    PlainYearMonth_prototype_properties,
    nullptr,
    ClassSpec::DontDefineConstructor,
};
