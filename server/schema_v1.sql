DROP TABLE IF EXISTS covers;
DROP TABLE IF EXISTS entries;

CREATE TABLE entries
(
	_id INTEGER PRIMARY KEY AUTOINCREMENT,
	filepath TEXT,
	item_class INTEGER NOT NULL DEFAULT(0),
	parent INTEGER NOT NULL DEFAULT(0),
	last_write_time INTEGER NOT NULL,
	org_pn TEXT,
	mime TEXT,
	title TEXT NOT NULL,
	artist TEXT,
	album_artist TEXT,
	composer TEXT,
	album TEXT,
	genre TEXT,
	date TEXT,
	comment TEXT,
	track INTEGER NOT NULL DEFAULT(0),
	size INTEGER NOT NULL DEFAULT(0),
	bitrate INTEGER NOT NULL DEFAULT(0),
	duration INTEGER NOT NULL DEFAULT(0),
	sample_freq INTEGER NOT NULL DEFAULT(0),
	channels INTEGER NOT NULL DEFAULT(0),
	width INTEGER NOT NULL DEFAULT(0),
	height INTEGER NOT NULL DEFAULT(0),
	bps INTEGER NOT NULL DEFAULT(0),

	FOREIGN KEY (parent) REFERENCES entries(_id) ON DELETE CASCADE
);

CREATE TABLE covers
(
	entry_id INTEGER UNIQUE REFERENCES entries(_id) ON DELETE CASCADE,
	org_pn TEXT NOT NULL,
	mime TEXT NOT NULL,
	width INTEGER NOT NULL,
	height INTEGER NOT NULL
);

-- Unknown,
-- Image,
-- Audio,
-- Video,
-- Container

INSERT INTO entries (_id, item_class, parent, last_write_time, title) VALUES (-1, 0, -1, 0, "dummy");
INSERT INTO entries (_id, item_class, parent, last_write_time, title) VALUES ( 0, 4, -1, 0, "root");
