**AirC remote DB**
To run program locate into: **cd ./bin/server/** and run **./server**
Default table in: ./bin/server/db/air.json

Server will start on 127.0.0.1:4000, to get access you can use any HTTP client.

**DB requests**
To get data send GET HTTP request with JSON body, ex: 
```
{
    "target": "db",
    "table": "air"
}
```
![Get all data](https://github.com/licurg/airc-db/blob/master/screenshots/Screenshot from 2020-04-25 17-55-36.png)

```
{
    "target": "db",
    "table": "air",
    "index": 0
}
```
![Get all data](https://github.com/licurg/airc-db/blob/master/screenshots/Screenshot from 2020-04-25 18-24-54.png)

```
{
    "target": "db",
    "table": "air",
    "search": [
        {"key": "longitude", "value": 30.5238}
    ]
}
```
![Get all data](https://github.com/licurg/airc-db/blob/master/screenshots/Screenshot from 2020-04-25 18-26-58.png)

To change data send POST HTTP request with JSON body, ex:
```
{
    "target": "db",
    "table": "air",
    "index": 0,
    "data": [
        {"key": "id", "value": 2}
    ]
}
```
![Get all data](https://github.com/licurg/airc-db/blob/master/screenshots/Screenshot from 2020-04-25 18-02-27.png)

To put data send PUT HTTP request with JSON body, ex:
```
{
    "target": "db",
    "table": "air",
    "row": {
        "id": 2,
        "area": "Los Angeles",
        "latitude": 40.3333,
        "longitude": 36.3333,
        "value": 0.2
    }
}
```
![Get all data](https://github.com/licurg/airc-db/blob/master/screenshots/Screenshot from 2020-04-25 18-05-45.png)

To delete data send DELETE HTTP request with JSON body, ex:
```
{
    "target": "db",
    "table": "air",
    "index": 0
}
```
![Delete data with search](https://github.com/licurg/airc-db/blob/master/screenshots/Screenshot from 2020-04-25 18-12-50.png)

```
{
    "target": "db",
    "table": "air",
    "search": [
        {"key": "id", "value": 3}
    ]
}
```
![Delete data with search](https://github.com/licurg/airc-db/blob/master/screenshots/Screenshot from 2020-04-25 18-10-19.png)
