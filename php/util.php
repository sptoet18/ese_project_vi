<?php
    // Verify Password
    function isCorrectPassword($password, $hashedPassword) {
        if (password_hash($password, PASSWORD_DEFAULT) === $hashedPassword) {
            return true;
        } else {
            return false;
        }
    }
?>