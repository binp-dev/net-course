FROM python:latest

COPY . /srv

WORKDIR /srv

RUN pip install -r requirements.txt

CMD flask --app server run -h 0.0.0.0 -p 80
