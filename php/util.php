<?php
    // --------- Database ---------
    function dbConnect(string $path, string $user, string $password) {
        $db = new PDO($path, $user, $password);
        $db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);

        return $db;
    }

    // --------- Passwords ---------

    // Verify Password
    function isCorrectPassword(string $password, string $hashedPassword) : bool {
        if (password_hash($password, PASSWORD_DEFAULT) === $hashedPassword) {
            return true;
        } else {
            return false;
        }
    }
?>