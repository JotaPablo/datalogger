import pandas as pd
import matplotlib.pyplot as plt
import os

filename = "datalog.csv"

if not os.path.exists(filename):
    print(f"Arquivo '{filename}' não encontrado!")
    exit(1)


# Leia o arquivo CSV
df = pd.read_csv(filename)

# Gráfico do acelerômetro
plt.figure(figsize=(10, 5))
plt.plot(df["amostra"], df["accel_x"], label="Acc X")
plt.plot(df["amostra"], df["accel_y"], label="Acc Y")
plt.plot(df["amostra"], df["accel_z"], label="Acc Z")
plt.title("Acelerômetro")
plt.xlabel("Amostra")
plt.ylabel("Aceleração (g)")
plt.legend()
plt.grid()
plt.tight_layout()
plt.show()

# Gráfico do giroscópio
plt.figure(figsize=(10, 5))
plt.plot(df["amostra"], df["gyro_x"], label="Gyro X")
plt.plot(df["amostra"], df["gyro_y"], label="Gyro Y")
plt.plot(df["amostra"], df["gyro_z"], label="Gyro Z")
plt.title("Giroscópio")
plt.xlabel("Amostra")
plt.ylabel("Velocidade Angular (°/s)")
plt.legend()
plt.grid()
plt.tight_layout()
plt.show()
