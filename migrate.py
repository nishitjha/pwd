import sys
import os
import getpass
import hashlib
import hmac
import subprocess

home = os.path.expanduser("~")
secretsPath = os.path.join(home, ".pwd_secrets.txt")
saltPath = os.path.join(home, ".pwd_salt")
backupPath = secretsPath + ".old"
oldKey = bytes.fromhex("2b7e151628aed2a6abf7158809cf4f3c")
oldIv = bytes.fromhex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff")
iterations = 1000000


def derive(password, salt):
    return hashlib.pbkdf2_hmac("sha1", password, salt, iterations, 16)


def ctr(key, iv, data):
    result = subprocess.run(
        ["openssl", "enc", "-aes-128-ctr", "-K", key.hex(), "-iv", iv.hex()],
        input=data, capture_output=True, check=True)
    return result.stdout


def computeMac(key, iv, cipher):
    macKey = hmac.new(key, b"pwd-mac", "sha1").digest()
    return hmac.new(macKey, iv + cipher, "sha1").digest()


def looksValid(data):
    if len(data) == 0 or not data.endswith(b"\n"):
        return False
    for b in data:
        if b != 10 and (b < 32 or b > 126):
            return False
    for line in data.split(b"\n")[:-1]:
        if len(line) > 0 and b":" not in line:
            return False
    return True


def writeSafely(path, data):
    tmp = path + ".tmp"
    fd = os.open(tmp, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(fd, "wb") as f:
        f.write(data)
    os.chmod(tmp, 0o600)
    os.replace(tmp, path)


def main():
    if not os.path.exists(secretsPath):
        print("No secrets file found, nothing to migrate.")
        return 1

    with open(secretsPath, "rb") as f:
        raw = f.read()

    plain = ctr(oldKey, oldIv, raw)
    if not looksValid(plain):
        print("Decryption gave garbage, this doesn't look like an old-format file. Nothing was changed.")
        return 1

    print("Old file decrypted fine. Pick the master password for the new vault.")
    password = getpass.getpass("New master password: ").encode()
    if len(password) == 0:
        print("Password can't be empty. Nothing was changed.")
        return 1
    if getpass.getpass("Again: ").encode() != password:
        print("Passwords didn't match. Nothing was changed.")
        return 1

    fd = os.open(backupPath, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(fd, "wb") as f:
        f.write(raw)

    if os.path.exists(saltPath):
        with open(saltPath, "rb") as f:
            salt = f.read()
        if len(salt) != 16:
            print("Existing ~/.pwd_salt is the wrong size, aborting. Nothing was changed.")
            return 1
    else:
        salt = os.urandom(16)
        writeSafely(saltPath, salt)

    key = derive(password, salt)
    iv = os.urandom(16)
    cipher = ctr(key, iv, plain)
    writeSafely(secretsPath, iv + cipher + computeMac(key, iv, cipher))

    print("Migrated. Old file kept at " + backupPath + ", delete it once you've checked everything works.")
    return 0


sys.exit(main())