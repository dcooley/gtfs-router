#v 0.0.1.00x

Major changes:

- New function `frequencies_to_stop_times` thanks to new co-author @stmarcin,
  and new contributor @AlexandraKapp
- Data without `transfers.txt` now load rather than error
- New function `gtfs_transfer_table` makes transfer table for feeds which
  contain no such table; see #14
- Former errors in reading of `zip` archives on Windows OS fixed

Minor changes:

- `extract_gtfs` has new parameter, `stn_suffixes`, to enable specification of
  any suffixes to be optionally removed from station IDs (#37; thanks to
  AlexandraKapp).
- Bug fix in station name matches in `gtfs_route` fn (see #26)
- `gtfs_route` accepts `stop_id` values as well as `stop_name` (see #26)
- `gtfs_isochrone` accepts equivalent `stop_id` values via `from_is_id` parameter.
- both `gtfs_route` and `gtfs_isochrone` accept (lon, lat) values for from and
  to stations.
- `gtfs_isochrone` returns `stop_id` as well as `stop_name` values (#29).