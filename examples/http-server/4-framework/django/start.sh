#!/bin/sh

poetry run ./manage.py migrate && \
poetry run ./manage.py runserver
