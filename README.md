# Tide (Alpha)
Tide is a m3u8 video downloader written in C++. It first reads the url of m3u8 file supplied by user, then downloads all video segments into a single file for local playback using libcurl.

# Tide-MPD (Alpha)
Tide-MPD, on the other hand, works with streams in MPEG-DASH MPD format. It reads the url of mpd file supplied by user, then downloads all video and audio segments into two files, and finally merge them together with ffmpeg.

## Usage
```
./tide url [output_name]
./tide-mpd url [output_name]
```

## Build
Tide has been tested on macOS El Capitan and Sierra, as well as Ubuntu 16.04 LTS.
```
g++ --std=c++14 tide.cpp -o tide -lcurl
g++ --std=c++14 tide-mpd.cpp -o tide-mpd -lcurl -lxerces-c
```

## Limitations (Tide)
Since Tide is currently only at Alpha stage, there are a few known limitations and restrictions:
- The m3u8 file supplied through command line needs to be one that contains m3u8 paths for different resolutions. The script will automatically fetch the highest quality determined by the average bandwidth specification. An example would be.
```
#EXTM3U
#EXT-X-STREAM-INF:AVERAGE-BANDWIDTH=178271,BANDWIDTH=2785909,CODECS="mp4a.40.5,avc1.4d401f",RESOLUTION=640x360,FRAME-RATE=25.000
360p/play.m3u8
#EXT-X-STREAM-INF:AVERAGE-BANDWIDTH=289071,BANDWIDTH=4869450,CODECS="mp4a.40.5,avc1.640029",RESOLUTION=1280x720,FRAME-RATE=25.000
720p/play.m3u8
```
- All urls in m3u8 files must be relative.

## Limitations (Tide-MPD)
Similarly, Tide-MPD is currently at Alpha stage:
- It recognizes only mpd file with following characteristics in defining the video and audio sources, i.e., the hierarchy in xml format is as follows. The first quality option will be used. AdaptationSet for video should come before audio.
```
MPD -> Period -> AdaptationSet -> Representation -> SegmentTemplate
```
- A `video` and `audio` folder should be created before running the script.
- After the script finishes running, all segments will be stored inside either video or audio. Now it's time to run some commands manually to stich them and pass through ffmpeg. First one inside audio directory, second inside video directory, and third in main directory.
```
cat init.m4s $(ls -1v | egrep "[0-9]+.m4s") > audio.m4s
cat init.m4s $(ls -1v | egrep "[0-9]+.m4s") > video.m4s
ffmpeg -i video/video.m4s -i audio/audio.m4s -c:v libx264 -c:a aac -strict experimental -threads 6 out.mp4
```
