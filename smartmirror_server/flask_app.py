# Most features were added in random order during the development and after learning new ways, because of that logic might not be similar in all situations and code is not simplest possible
# Main structure is ok and device works as intended and the new fixes for simpler code and more features would be easy to implement

from flask import Flask,jsonify,request,render_template,redirect, url_for, session, flash  # last 4 are for login logic
from functools import wraps # login logic aswell
import datetime
import requests
import openmeteo_requests
import requests_cache
from retry_requests import retry
import pandas as pd
import pytz
import numpy as np
import sqlite3


# initiates the flask application and creates instance of Flask, later accessed with app.
app = Flask(__name__)

#################### Login logic ####################
# Since there is no sensitive information stored anywhere, data security is not main concern here but rather just limiting the access of the website


# secret key for signing session cookie, needs to be set for using sessions (cookies)
# for better security this should not be hardcoded, better way would be using environment variables
app.secret_key = "m6{NAmE9]a3Lzm~"

# function that acts as decorator to be applied to routes which need to be protected with login
def login_required(f):
    @wraps(f)  ## enables to not lose information about input function, needed because of behaviour of decorators
    def wrap(*args,**kwargs):
        if 'logged_in' in session:
            return f(*args, **kwargs)
        else:
            flash('You need to login...')
            return redirect(url_for('login')) #redirect to login page
    return wrap


# route for login page and function to be invoked when accessed
@app.route('/login', methods=['GET', 'POST'])
def login():
    error = None
    if request.method == 'POST':
        if request.form['username'] != 'fresh' or request.form['password'] != 'air': #requiring password from the client with a html form, hardcoding not the best way to do it
            error = 'Invalid Credentials. Please try again.'
        else:
            session['logged_in']=True
            return redirect(url_for('index')) #redirect to index page
    return render_template('login.html', error=error)
## route and function for logout route
@app.route('/logout')
def logout():
    session.pop('logged_in',None) # drops the 'logged in' key from session data so that server knows the user is not logged in anymore
    return redirect(url_for('login')) #redirect to login page

#####################################################


#################### database and related functions ####################
# Better way would be just having single database and accessing specific parts of it but because some features where implemented in  order there is now couple of different simple databases
# Also connecting to database and closing it frequently is not the best way to do this, but making multiple changes in single opening would be best for performance and concurrency (problems with simultaneous updates etc.)




#### current stop db ###
#structure of the database in current_stop.db (stop is the table containing columns and rows):
'''

#stop
#id | gtfsId | coordinates
# 1 | HSL:12321 | 60.2031,2396

'''


#current stop is stored in database, this function get the id and latitude and longitude from that db
## returns in tuple of strings ("id","latlon")
def get_stop_db_value():
    conn = sqlite3.connect('/home/aaroesko/smartmirror_server/current_stop.db') #connection to database stored on server, returns object in conn variable
    cursor = conn.cursor() # cursor object is needed for executing commands and fetching data from the database
    cursor.execute('SELECT * FROM stop LIMIT 1')  # command to be applied on the database, SELECT = selection command that operates on the next statements, * = all, stop = name of the table in the db, LIMIT 1= returns only the first row
    db_info=cursor.fetchone() # storing the information, fetchone() returns single record as tuple
    current_gtfsId =db_info[1]  # index 1 is the stop id
    current_coordinates=db_info[2] ## index 2 is the coordinates as string separated by comma,  first lat second is lon
    conn.commit() # not necessary since no changes were made
    cursor.close() # closing the cursor object
    return (current_gtfsId,current_coordinates)


#function to update database
def update_stop_db(new_gtfsId,latlon):
    conn = sqlite3.connect('/home/aaroesko/smartmirror_server/current_stop.db')
    cursor = conn.cursor()
    # Update the value in the database
    cursor.execute('''
        UPDATE stop
        SET gtfsId = ?,
            coordinates = ?
    ''', (new_gtfsId, latlon))  ## new information updated to gtfsId and coordinates, marked by ? and their values defined in the parameters
    conn.commit()  # necessary for making changes
    cursor.close()




#### note db ###
#structure of the database in note.db (notes is the table containing columns and rows):
'''
#notes
#id | note
# 1 | "note 1"
# 2 | "note 2"
# 3 | "note 3"
'''
## returns in dict note_dict = { note1: "note1",note2: "note2",note3: "note3" }
def get_note_db():
    conn = sqlite3.connect('/home/aaroesko/smartmirror_server/notes.db')
    cursor = conn.cursor()
    cursor.execute('SELECT note FROM notes ORDER BY rowid ASC LIMIT 3') # select from note table ordered by ascendint rowid and limited to 3 rows
    notes = cursor.fetchall() # gets all rows as list of tuples
    conn.close()

    # Constructing a dictionary with note keys
    note_dict = {}
    for i, note in enumerate(notes):
        note_dict[f"note{i+1}"] = note[0]

    return note_dict

def update_note_db(new_note):
    conn = sqlite3.connect('/home/aaroesko/smartmirror_server/notes.db')
    cursor = conn.cursor()

    cursor.execute('SELECT note FROM notes') # fetches all notes to check how many we have
    current_notes = cursor.fetchall()

    # if there are already three notes, remove the oldest one since we decided to have only 3 notes
    if len(current_notes) >= 3:
        cursor.execute('DELETE FROM notes WHERE rowid = (SELECT MIN(rowid) FROM notes)') # delete the oldest note
        cursor.execute('UPDATE notes SET rowid = rowid - 1') # reset row IDs

    cursor.execute('INSERT INTO notes (note) VALUES (?)', (new_note,))#when only 3 notes, insert the new note
    conn.commit()
    conn.close()




### event db ###
# similar workings to that of notes
def get_event_db():
    conn = sqlite3.connect('/home/aaroesko/smartmirror_server/events.db')
    cursor = conn.cursor()
    cursor.execute('SELECT event FROM events ORDER BY rowid ASC LIMIT 3')
    events = cursor.fetchall()
    conn.close()

    # Constructing a dictionary with note keys
    event_dict = {}
    for i, event in enumerate(events):
        event_dict[f"event{i+1}"] = event[0]

    return event_dict

def update_event_db(new_event):
    conn = sqlite3.connect('/home/aaroesko/smartmirror_server/events.db')
    cursor = conn.cursor()

    cursor.execute('SELECT event FROM events')
    current_events = cursor.fetchall()

    if len(current_events) >= 3:
        cursor.execute('DELETE FROM events WHERE rowid = (SELECT MIN(rowid) FROM events)')
        cursor.execute('UPDATE events SET rowid = rowid - 1')

    cursor.execute('INSERT INTO events (event) VALUES (?)', (new_event,))

    conn.commit()
    conn.close()
#####################################################




#weather codes as dictionary, night or image information is not used in any way
weather_codes={
	"0":{
		"day":{
			"description":"Sunny",
			"image":"http://openweathermap.org/img/wn/01d@2x.png"
		},
		"night":{
			"description":"Clear",
			"image":"http://openweathermap.org/img/wn/01n@2x.png"
		}
	},
	"1":{
		"day":{
			"description":"Mainly Sunny",
			"image":"http://openweathermap.org/img/wn/01d@2x.png"
		},
		"night":{
			"description":"Mainly Clear",
			"image":"http://openweathermap.org/img/wn/01n@2x.png"
		}
	},
	"2":{
		"day":{
			"description":"Partly Cloudy",
			"image":"http://openweathermap.org/img/wn/02d@2x.png"
		},
		"night":{
			"description":"Partly Cloudy",
			"image":"http://openweathermap.org/img/wn/02n@2x.png"
		}
	},
	"3":{
		"day":{
			"description":"Cloudy",
			"image":"http://openweathermap.org/img/wn/03d@2x.png"
		},
		"night":{
			"description":"Cloudy",
			"image":"http://openweathermap.org/img/wn/03n@2x.png"
		}
	},
	"45":{
		"day":{
			"description":"Foggy",
			"image":"http://openweathermap.org/img/wn/50d@2x.png"
		},
		"night":{
			"description":"Foggy",
			"image":"http://openweathermap.org/img/wn/50n@2x.png"
		}
	},
	"48":{
		"day":{
			"description":"Rime Fog",
			"image":"http://openweathermap.org/img/wn/50d@2x.png"
		},
		"night":{
			"description":"Rime Fog",
			"image":"http://openweathermap.org/img/wn/50n@2x.png"
		}
	},
	"51":{
		"day":{
			"description":"Light Drizzle",
			"image":"http://openweathermap.org/img/wn/09d@2x.png"
		},
		"night":{
			"description":"Light Drizzle",
			"image":"http://openweathermap.org/img/wn/09n@2x.png"
		}
	},
	"53":{
		"day":{
			"description":"Drizzle",
			"image":"http://openweathermap.org/img/wn/09d@2x.png"
		},
		"night":{
			"description":"Drizzle",
			"image":"http://openweathermap.org/img/wn/09n@2x.png"
		}
	},
	"55":{
		"day":{
			"description":"Heavy Drizzle",
			"image":"http://openweathermap.org/img/wn/09d@2x.png"
		},
		"night":{
			"description":"Heavy Drizzle",
			"image":"http://openweathermap.org/img/wn/09n@2x.png"
		}
	},
	"56":{
		"day":{
			"description":"Light Freezing Drizzle",
			"image":"http://openweathermap.org/img/wn/09d@2x.png"
		},
		"night":{
			"description":"Light Freezing Drizzle",
			"image":"http://openweathermap.org/img/wn/09n@2x.png"
		}
	},
	"57":{
		"day":{
			"description":"Freezing Drizzle",
			"image":"http://openweathermap.org/img/wn/09d@2x.png"
		},
		"night":{
			"description":"Freezing Drizzle",
			"image":"http://openweathermap.org/img/wn/09n@2x.png"
		}
	},
	"61":{
		"day":{
			"description":"Light Rain",
			"image":"http://openweathermap.org/img/wn/10d@2x.png"
		},
		"night":{
			"description":"Light Rain",
			"image":"http://openweathermap.org/img/wn/10n@2x.png"
		}
	},
	"63":{
		"day":{
			"description":"Rain",
			"image":"http://openweathermap.org/img/wn/10d@2x.png"
		},
		"night":{
			"description":"Rain",
			"image":"http://openweathermap.org/img/wn/10n@2x.png"
		}
	},
	"65":{
		"day":{
			"description":"Heavy Rain",
			"image":"http://openweathermap.org/img/wn/10d@2x.png"
		},
		"night":{
			"description":"Heavy Rain",
			"image":"http://openweathermap.org/img/wn/10n@2x.png"
		}
	},
	"66":{
		"day":{
			"description":"Light Freezing Rain",
			"image":"http://openweathermap.org/img/wn/10d@2x.png"
		},
		"night":{
			"description":"Light Freezing Rain",
			"image":"http://openweathermap.org/img/wn/10n@2x.png"
		}
	},
	"67":{
		"day":{
			"description":"Freezing Rain",
			"image":"http://openweathermap.org/img/wn/10d@2x.png"
		},
		"night":{
			"description":"Freezing Rain",
			"image":"http://openweathermap.org/img/wn/10n@2x.png"
		}
	},
	"71":{
		"day":{
			"description":"Light Snow",
			"image":"http://openweathermap.org/img/wn/13d@2x.png"
		},
		"night":{
			"description":"Light Snow",
			"image":"http://openweathermap.org/img/wn/13n@2x.png"
		}
	},
	"73":{
		"day":{
			"description":"Snow",
			"image":"http://openweathermap.org/img/wn/13d@2x.png"
		},
		"night":{
			"description":"Snow",
			"image":"http://openweathermap.org/img/wn/13n@2x.png"
		}
	},
	"75":{
		"day":{
			"description":"Heavy Snow",
			"image":"http://openweathermap.org/img/wn/13d@2x.png"
		},
		"night":{
			"description":"Heavy Snow",
			"image":"http://openweathermap.org/img/wn/13n@2x.png"
		}
	},
	"77":{
		"day":{
			"description":"Snow Grains",
			"image":"http://openweathermap.org/img/wn/13d@2x.png"
		},
		"night":{
			"description":"Snow Grains",
			"image":"http://openweathermap.org/img/wn/13n@2x.png"
		}
	},
	"80":{
		"day":{
			"description":"Light Showers",
			"image":"http://openweathermap.org/img/wn/09d@2x.png"
		},
		"night":{
			"description":"Light Showers",
			"image":"http://openweathermap.org/img/wn/09n@2x.png"
		}
	},
	"81":{
		"day":{
			"description":"Showers",
			"image":"http://openweathermap.org/img/wn/09d@2x.png"
		},
		"night":{
			"description":"Showers",
			"image":"http://openweathermap.org/img/wn/09n@2x.png"
		}
	},
	"82":{
		"day":{
			"description":"Heavy Showers",
			"image":"http://openweathermap.org/img/wn/09d@2x.png"
		},
		"night":{
			"description":"Heavy Showers",
			"image":"http://openweathermap.org/img/wn/09n@2x.png"
		}
	},
	"85":{
		"day":{
			"description":"Light Snow Showers",
			"image":"http://openweathermap.org/img/wn/13d@2x.png"
		},
		"night":{
			"description":"Light Snow Showers",
			"image":"http://openweathermap.org/img/wn/13n@2x.png"
		}
	},
	"86":{
		"day":{
			"description":"Snow Showers",
			"image":"http://openweathermap.org/img/wn/13d@2x.png"
		},
		"night":{
			"description":"Snow Showers",
			"image":"http://openweathermap.org/img/wn/13n@2x.png"
		}
	},
	"95":{
		"day":{
			"description":"Thunderstorm",
			"image":"http://openweathermap.org/img/wn/11d@2x.png"
		},
		"night":{
			"description":"Thunderstorm",
			"image":"http://openweathermap.org/img/wn/11n@2x.png"
		}
	},
	"96":{
		"day":{
			"description":"Light Thunderstorms With Hail",
			"image":"http://openweathermap.org/img/wn/11d@2x.png"
		},
		"night":{
			"description":"Light Thunderstorms With Hail",
			"image":"http://openweathermap.org/img/wn/11n@2x.png"
		}
	},
	"99":{
		"day":{
			"description":"Thunderstorm With Hail",
			"image":"http://openweathermap.org/img/wn/11d@2x.png"
		},
		"night":{
			"description":"Thunderstorm With Hail",
			"image":"http://openweathermap.org/img/wn/11n@2x.png"
		}
	}
}

## function to get specific weather code from the weather codes dictionary defined above
### returns "Cloudy" for example based on give code as parameter
def get_weather_condition(code):
    sunny_list = [0, 1]
    partly_cloudy_list = [2]
    cloudy_list=[3]
    foggy_list = [45, 48]
    rain_list = [51,53, 43, 55, 56, 57, 61, 63, 65, 67, 80, 81, 85]
    snow_list = [73, 71, 75, 77, 86]
    thunder_list = [95, 96, 99]

    if code in sunny_list:
        return "Sunny"
    elif code in cloudy_list:
        return "Cloudy"
    elif code in partly_cloudy_list:
        return "Half cloudy"
    elif code in foggy_list:
        return "Foggy"
    elif code in rain_list:
        return "Rainy"
    elif code in snow_list:
        return "Snowy"
    elif code in thunder_list:
        return "Thunder"
    else:
        return "Unknown"



### function for weather data infromation from openmeteo api
## parameters are coordinates of location
##returns dict of form weather_data_dict = {cloud_cover: 0 , date: '2024-05-19 18:00:00'}
##  timezone should be dynamic aswell but currently only in Europe/Moscow
def get_weather_data(lat:float,lon:float):
    cache_session = requests_cache.CachedSession('.cache', expire_after = 3600)
    retry_session = retry(cache_session, retries = 5, backoff_factor = 0.2)
    openmeteo = openmeteo_requests.Client(session = retry_session)

    url = "https://api.open-meteo.com/v1/forecast"
    params = {
    	"latitude": lat,
    	"longitude": lon,
    	"hourly": ["temperature_2m", "precipitation_probability", "weather_code", "cloud_cover"],
    	"wind_speed_unit": "ms",
    	"timezone": "Europe/Moscow",
    	"forecast_days": 2
    }
    responses = openmeteo.weather_api(url, params=params)

    response = responses[0]
    print(f"Coordinates {response.Latitude()}°N {response.Longitude()}°E")
    print(f"Elevation {response.Elevation()} m asl")
    print(f"Timezone {response.Timezone()} {response.TimezoneAbbreviation()}")
    print(f"Timezone difference to GMT+0 {response.UtcOffsetSeconds()} s")


    hourly = response.Hourly()
    hourly_temperature_2m = hourly.Variables(0).ValuesAsNumpy()
    hourly_precipitation_probability = hourly.Variables(1).ValuesAsNumpy()
    hourly_weather_code = hourly.Variables(2).ValuesAsNumpy()
    hourly_cloud_cover = hourly.Variables(3).ValuesAsNumpy()

    hourly_data = {"date": pd.date_range(
    	start = pd.to_datetime(hourly.Time(), unit = "s"),
    	end = pd.to_datetime(hourly.TimeEnd(), unit = "s"),
    	freq = pd.Timedelta(seconds = hourly.Interval()),
    	inclusive = "left"
    )}
    hourly_data["temperature_2m"] = hourly_temperature_2m
    hourly_data["precipitation_probability"] = hourly_precipitation_probability
    hourly_data["weather_code"] = hourly_weather_code
    hourly_data["cloud_cover"] = hourly_cloud_cover
    hourly_dataframe = pd.DataFrame(data = hourly_data)
    df=hourly_dataframe

    #spagetti
    current_time=(datetime.datetime.now())
    desired_timezone = pytz.timezone('Europe/Moscow')
    current_time = datetime.datetime.now(desired_timezone)
    current_time=current_time.replace(tzinfo=None)
    rounded_time = current_time.replace(second=0, microsecond=0, minute=0) + datetime.timedelta(hours=1) if current_time.minute >= 30 else current_time.replace(second=0, microsecond=0, minute=0)
    current_time=rounded_time
    current_time=pd.to_datetime(current_time)
    weather_data_series = df[df.iloc[:,0]==current_time]
    weather_data_series['temperature_2m'] = weather_data_series['temperature_2m'].round(1).astype(str)

    weather_code=weather_data_series.to_numpy()[0][3]

    weather_data_dict=weather_data_series.to_dict()
    new_data = {}
    for key, value in weather_data_dict.items():
        new_data[key] = list(value.values())[0]

    weather_data_dict=new_data
    weather_data_dict["weather_code"]=get_weather_condition(weather_code)

    stringified_timestamp = weather_data_dict["date"].strftime('%Y-%m-%d %H:%M:%S')
    weather_data_dict["date"]=stringified_timestamp

    return weather_data_dict

# spagetti function for getting dictionary for secondary weather data hourly dataframe
def create_secondary_weather_dict(df, n):
	current_time = datetime.datetime.now()
	desired_timezone = pytz.timezone('Europe/Moscow')
	current_time = datetime.datetime.now(desired_timezone)
	current_time = current_time.replace(tzinfo=None)
	rounded_time = current_time.replace(second=0, microsecond=0, minute=0) + datetime.timedelta(hours=1) if current_time.minute >= 30 else current_time.replace(second=0, microsecond=0, minute=0)
	current_time = rounded_time
	current_time = pd.to_datetime(current_time)

	nhour = current_time + pd.Timedelta(hours=n)
	nhour = pd.to_datetime(nhour)
	weather_data_series = df[df.iloc[:, 0] == nhour]

	weather_code = weather_data_series.to_numpy()[0][2]
	weather_data_dict = weather_data_series.to_dict()
	new_data = {}
	for key, value in weather_data_dict.items():
		new_data[key] = list(value.values())[0]
	weather_data_dict = new_data

	weather_data_dict["weather_code"] = get_weather_condition(weather_code)
	stringified_timestamp = weather_data_dict["date"].strftime('%H:%M')
	weather_data_dict["date"] = stringified_timestamp

	return weather_data_dict

#another api request for secondary weather data, could also be included on the first one but resolving old spagetti code was slower than making a new api request
# includes some very peculiar fixes that work on this case
def get_secondary_weather_data(lat: float, lon: float):
    # Setup the Open-Meteo API client with cache and retry on error
    cache_session = requests_cache.CachedSession('.cache', expire_after=3600)
    retry_session = retry(cache_session, retries=5, backoff_factor=0.2)
    openmeteo = openmeteo_requests.Client(session=retry_session)

    # Make sure all required weather variables are listed here
    # The order of variables in hourly or daily is important to assign them correctly below
    url = "https://api.open-meteo.com/v1/forecast"
    params = {
        "latitude": lat,
        "longitude": lon,
		"hourly": ["temperature_2m", "precipitation_probability", "weather_code", "wind_speed_10m"],
        "wind_speed_unit": "ms",
		"daily": ["weather_code", "temperature_2m_max", "precipitation_probability_max", "wind_speed_10m_max"],
        "timezone": "Europe/Moscow",
        "forecast_days": 4
    }
    responses = openmeteo.weather_api(url, params=params)

    # Process first location. Add a for-loop for multiple locations or weather models
    response = responses[0]

    hourly = response.Hourly()
    hourly_temperature_2m = hourly.Variables(0).ValuesAsNumpy()
    hourly_precipitation_probability = hourly.Variables(1).ValuesAsNumpy()
    hourly_weather_code = hourly.Variables(2).ValuesAsNumpy()
    hourly_wind_speed_10m = hourly.Variables(3).ValuesAsNumpy()

    hourly_data = {"date": pd.date_range(
        start=pd.to_datetime(hourly.Time(), unit="s"),
        end=pd.to_datetime(hourly.TimeEnd(), unit="s"),
        freq=pd.Timedelta(seconds=hourly.Interval()),
        inclusive="left"
    )}
    hourly_temperature_2m = hourly_temperature_2m.round(decimals=1)
    hourly_data["precipitation_probability"] = hourly_precipitation_probability
    hourly_data["weather_code"] = hourly_weather_code
    hourly_wind_speed_10m=hourly_wind_speed_10m.round(decimals=1)

	### fix for for temperature floats
    strlist=[]
    for i in hourly_temperature_2m:
        strlist.append(str(i))

    hourly_data["temperature_2m"] = strlist

    strlist=[]
    for i in hourly_wind_speed_10m:
        strlist.append(str(i))

    hourly_data["hourly_wind_speed_10m"] = strlist


    hourly_dataframe = pd.DataFrame(data=hourly_data)
    df_hourly = hourly_dataframe


    daily = response.Daily()
    daily_weather_code = daily.Variables(0).ValuesAsNumpy()
    daily_temperature_2m_max = daily.Variables(1).ValuesAsNumpy()
    daily_precipitation_probability_max = daily.Variables(2).ValuesAsNumpy()
    daily_wind_speed_10m_max = daily.Variables(3).ValuesAsNumpy()


    daily_data = {"date": pd.date_range(
		start = pd.to_datetime(daily.Time(), unit = "s"),
		end = pd.to_datetime(daily.TimeEnd(), unit = "s"),
		freq = pd.Timedelta(seconds = daily.Interval()),
		inclusive = "left"
	)}
    daily_data["weather_code"] = get_weather_condition(daily_weather_code[0])
    daily_temperature_2m_max=daily_temperature_2m_max.round(decimals=1)
    daily_precipitation_probability_max= daily_precipitation_probability_max.round(decimals=1)
    daily_wind_speed_10m_max = daily_wind_speed_10m_max.round(decimals=1)


	# temp fix floats
    strlist=[]
    for i in daily_temperature_2m_max:
        strlist.append(str(i))

    daily_data["temperature_2m_max"] = strlist

	# precip fix
    strlist=[]
    for i in daily_precipitation_probability_max:
        strlist.append(str(i))

    daily_data["daily_precipitation_probability_max"] = strlist


	# windspeed fix
    strlist=[]
    for i in daily_wind_speed_10m_max:
        strlist.append(str(i))

    daily_data["daily_wind_speed_10m_max"] = strlist

    daily_dataframe = pd.DataFrame(data = daily_data)
    df=daily_dataframe

    df['date'] = pd.to_datetime(df['date'])

    df['date'] = df['date'] + pd.DateOffset(days=1)

    df['date'] = df['date'].dt.strftime('%a')

    weather_dict = df.to_dict(orient='records')

    secondary_weather_dict={}
    secondary_weather_dict['weather_plus0']=weather_dict[0]
    secondary_weather_dict['weather_plus1']=weather_dict[1]
    secondary_weather_dict['weather_plus2']=weather_dict[2]
    secondary_weather_dict['weather_plus3']=weather_dict[3]

    # looks for next 12 hours with 3 hour steps
    for i in range(0,12,3):
         secondary_weather_dict[f'{i}']=create_secondary_weather_dict(df_hourly,i)

    return secondary_weather_dict


####################################### Website interrractions ######################################################

@app.route('/transport-receive',methods=['POST'])
def invoke_flask_function():
    data = request.json
    client_given_stop_name = data.get('data')
    stop_dict=get_stop_id(client_given_stop_name)

    formatted_data = [{"value": key, "label": f"{label}: {value}"} for key, (label, value) in stop_dict.items()]
    return jsonify({"choices": formatted_data})



@app.route('/send-selected-value',methods=['POST'])
def return_timetables():
    data = request.json
    client_given_stop_id = data.get('data')  # Assuming the key in the JSON is 'data'

    deps=get_departures(client_given_stop_id)
    coordinates=list(deps.values())[0][2]

    new_dict = {}
    for key, value in deps.items():
        new_dict[key] = value[0]

    update_stop_db(client_given_stop_id,coordinates) ## updates database...
    return jsonify(deps)


#update notes based on website modifications
@app.route('/note-receive',methods=['POST'])
def update_the_note():
    data=request.json
    new_message=data.get('data')
    update_note_db(new_message)

    return data


#route and function for alarm event
@app.route('/event_submission', methods=['POST'])
def alarm_event():
    data = request.get_json()
    new_event=data['event']
    update_event_db(new_event)
    return jsonify({'data': data})


 ################################## Functions for public transport requests##################################

### this returns stop ids and names
## in form: {'HSL:1230109': ('Kumpulan kampus', 'H3037'), 'HSL:1230112': ('Kumpulan kampus', 'H3036'), 'HSL:1240118': ('Kumpulan kampus', 'H3028'), 'HSL:1240103': ('Kumpulan kampus', 'H3029')
def get_stop_id(stop_name):
    # graphQL query with variables
    body = """
    query Stops($name: String!) {
      stops(name: $name) {
        gtfsId
        name
        code
      }
    }
    """

    #Subscription token from Digitransit, can be aquired with sign-in
    subscription_key = '687e4f332f994e9c9678edc8655ff3e7'

    #End point of the api request
    #Multiple different one available for different regions and versions
    api_url = 'https://api.digitransit.fi/routing/v1/routers/hsl/index/graphql'

    #headers containing the subscription key and content type
    headers = {
        'digitransit-subscription-key': subscription_key,
        'Content-Type': 'application/json'  # Adjust content type if necessary
    }


    #Variables for the query
    variables = {"name": stop_name}

    #The request itself
    response = requests.post(url=api_url, headers=headers, json={'query': body, 'variables': variables})

    json_response = response.json()

    #Dict for return
    gtfsid_info_dict = {stop["gtfsId"]: (stop["name"], stop["code"]) for stop in json_response["data"]["stops"]}

    return gtfsid_info_dict


#Get departures based on stop id
# Does API request
# One of the first implementations so the simplicity of the returned data is not optimal but works
def get_departures(stop_id):
    #The request body includes GraphQl query with variable stopId
    body="""
    query StopQuery($stopId: String!)
    {
    stop(id: $stopId) {
        stoptimesWithoutPatterns(numberOfDepartures: 3) {
        stop {
            platformCode
            lat
          	lon
        }
        realtimeArrival
        scheduledArrival
        trip {
            route {
            shortName
            }
        }
        headsign
        }
    }
    }
    """

    #Subscription token from Digitransit, can be aquired with sign-in
    subscription_key = '687e4f332f994e9c9678edc8655ff3e7'

    #End point of the api request
    #Multiple different one available for different regions and versions
    api_url = 'https://api.digitransit.fi/routing/v1/routers/hsl/index/graphql'

    #headers containing the subscription key and content type
    headers = {
        'digitransit-subscription-key': subscription_key,
        'Content-Type': 'application/json'  # Adjust content type if necessary
    }

    #Variables for the query
    variables = {"stopId": stop_id}

    #The request itself
    response = requests.post(url=api_url, headers=headers, json={'query': body, 'variables': variables})
    json_response=response.json() #get json on the response

    result = {} #dict eventually to be returned
    #Some datetime fixing
    current_time = datetime.datetime.now()
    desired_timezone = pytz.timezone('Europe/Moscow')
    current_time = datetime.datetime.now(desired_timezone)
    current_time=current_time.replace(tzinfo=None)
    midnight = current_time.replace(hour=0, minute=0, second=0, microsecond=0)
    time_difference = current_time - midnight
    time_difference_seconds = time_difference.total_seconds()


    #parsing the response
    # iterate through each entry in stoptimesWithoutPatterns
    for entry in json_response["data"]["stop"]["stoptimesWithoutPatterns"]:
        short_name = entry["trip"]["route"]["shortName"]
        realtime_arrival = entry["realtimeArrival"]
        headsign_label = entry["headsign"]

        time_difference_minutes=abs(round((time_difference_seconds-realtime_arrival)/60,0)) #query returns time difference for departure in seconds so change it to minutes
        str_time_difference_minutes=f'{int(time_difference_minutes)} min'

        formatted_arrival=str_time_difference_minutes

        shortname = f"{short_name}"
        headsign=f"{headsign_label}"

        lat=entry["stop"]["lat"]
        lon=entry["stop"]["lon"]
        latlon=f"{lat},{lon}"

        result[formatted_arrival] = [shortname,headsign,latlon]
    return result

################################################################################################################


#function for time data
def get_time_data():
        # time data
        # another timezone fix because the actualy server has different timezone returning wrong timezone on the datetime.now() function
        desired_timezone = pytz.timezone('Europe/Moscow')
        current_time = datetime.datetime.now(desired_timezone)
        current_time=current_time.replace(tzinfo=None)
        time_data={
            "year": current_time.year,
            "month": current_time.month,
            "day": current_time.day,
            "hours": current_time.hour,
            "mins": current_time.minute,
            "sec": current_time.second,
            }
        return time_data

###### DHT #######
# Atm the dht data is just stored as a variable on to the server, with initial value being Nan for each
# DHT data could actually be also be saved on database to have data over longer periods of time, not just real time
# Plots could actually also be easily made on the website with javascript libraries like plotly.js etc.
latest_dht_data = {
    'sensor': 'Nan',
    'value1': 'Nan',
    'value2': 'Nan'
}


#route for DHT_data
#Post for ESP32 and GET for website toget the data
@app.route("/DHT_data", methods=['POST', 'GET'])
def DHT_data():
    global latest_dht_data

    if request.method == 'POST':
        #data = request.headers
        data=request.json
        sensor = data.get('sensor')
        value1 = data.get('value1')
        value2 = data.get('value2')

        # Update the latest DHT data
        latest_dht_data = {
            'sensor': sensor,
            'value1': value1,
            'value2': value2
        }


        return latest_dht_data, 200 #return data and code 200 for success

    elif request.method == 'GET':
        return jsonify(latest_dht_data)

    else:
        return 400 #return 400 for failed request

###### Main json #####
# ESP32 request this route and receives json file as response
# Includes everything that we want to show on ESP32 from the server side
# Code would be more readable if the all the data was parsed already in separated functions and just returned here in dicts like is the case with time_data() etc.
@app.route('/main_data',methods=['GET'])
def return_main_data():
    if(request.method == 'GET'):

        if 'Kumpula' in request.args:  #  Query param. 'Kumpula' on the route so that access is limited
            global latest_dht_data

            #transport data and parsing
            current_stop_data=get_stop_db_value()
            current_stop_id=current_stop_data[0]
            current_stop_latlon=current_stop_data[1]
            current_stop_latlon = current_stop_latlon.strip("()").split(",")
            lat=float(current_stop_latlon[0])
            lon=float(current_stop_latlon[1])
            public_transport_times=get_departures(current_stop_id) #includes API request
            parsed_transport = {}
            for key, value in public_transport_times.items():
                parsed_transport[key] = value[0]

            #time data
            time_data=get_time_data()

            # note data
            note=get_note_db()

            #weather data
            weather=get_weather_data(lat,lon)#API request included

            #secondary weather data and parsing
            secondary_weather=get_secondary_weather_data(lat,lon) #API request included
            data_keys=list(public_transport_times.keys())
            data_values=list(public_transport_times.values())
            departure_dict={}
            for i in range(len(data_keys)):
                departure_dict[f'{i}']={'departure':data_keys[i],'shortname':data_values[i][0],'headsign':data_values[i][1]}

            #alarm event and parsing
            events_data=get_event_db()
            new_event_dict={}
            for event_i in events_data:
                event1_parsed_list=events_data[event_i].split(",")
                event_i_data_dict={

            	"year":event1_parsed_list[0],
            	"month":event1_parsed_list[1],
            	"day":event1_parsed_list[2],
            	"hour":event1_parsed_list[3],
            	"min":event1_parsed_list[4]
            	}
                new_event_dict[event_i]=event_i_data_dict


            # all the data should be in this dict
            data = {
                "time_data":time_data,
                "public_transport_data" : departure_dict,
                "weather_data": weather,
                "image_data": "( ͡° ͜ʖ ͡°)",
                "note":note,
                "secondary_weather_data":secondary_weather,
                "dht_data":latest_dht_data,
                "events":new_event_dict,

                    }


            return jsonify(data) ## whats returned to ESP32, jsonify used to wrap this up in json format

        else:
            return "UNAUTHORIZED", 400 # if query param not correct, return message and code 400 for bad request
##############################################################################################################################


# front page
@app.route("/")
@login_required
def index():
    return render_template("index2.html")



