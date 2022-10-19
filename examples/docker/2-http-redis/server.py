import redis
from flask import Flask

app = Flask(__name__)
cache = redis.Redis(host="redis", port=6379)


def get_hit_count():
    return cache.incr("hits")


@app.route("/")
def hello():
    count = get_hit_count()
    return "<h1>Hello World!</h1><p>I have been seen {} times.</p>".format(count)
