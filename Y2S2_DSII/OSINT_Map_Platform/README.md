# OSINT Map Platform

**Project:** OSINT Map Platform  
**Subject:** Y2S2_DSII – Database Systems II  
**Technology:** Python, Django, Leaflet.js, PostgreSQL  
**Student:** SAI0044 | VŠB-TUO

---

## Goal

Develop a web-based OSINT (Open Source Intelligence) mapping platform using Django. The platform aggregates publicly available data and visualizes it on an interactive map, enabling cyber security analysts to identify and track entities of interest.

---

## Features

- [ ] Interactive map interface powered by Leaflet.js and OpenStreetMap
- [ ] Django backend with REST API for data ingestion and retrieval
- [ ] Database models for storing geolocated OSINT entries
- [ ] Search and filter panel (by type, date, keyword)
- [ ] User authentication and role-based access control
- [ ] Export data to GeoJSON / CSV

---

## Setup

### Prerequisites

- Python 3.10+
- pip / virtualenv
- PostgreSQL (or SQLite for development)

### Installation

```bash
# Clone the repository
git clone https://github.com/deavers/University_Projects.git
cd University_Projects/Y2S2_DSII/OSINT_Map_Platform

# Create and activate a virtual environment
python -m venv venv
source venv/bin/activate      # Linux/macOS
# venv\Scripts\activate       # Windows

# Install dependencies
pip install -r requirements.txt

# Configure environment variables
cp .env.example .env
# Edit .env with your DATABASE_URL and SECRET_KEY

# Run migrations and start the development server
python manage.py migrate
python manage.py runserver
```

Open your browser at `http://127.0.0.1:8000/`.

---

## Screenshots

> *(Screenshots will be added once the UI is implemented.)*

---

## Notes

- Project developed as part of the **Database Systems II** course at VŠB-TUO.
- Sensitive files (`db.sqlite3`, `.env`, `venv/`) are excluded via root `.gitignore`.
