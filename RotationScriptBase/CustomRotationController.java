// frameworks/base/services/core/java/com/android/server/CustomRotationController.java

package com.android.server;

import android.content.Context;
import android.util.Slog;
import android.view.Surface;

import com.android.internal.view.RotationPolicy;


public class CustomRotationController {
    

    private static final String TAG = "CustomRotationController";

    /**
     *
     * @param context O contexto do sistema.
     * @param rotation A rotação desejada. Use as constantes de android.view.Surface:
     * - Surface.ROTATION_0 (Retrato normal, 0 graus)
     * - Surface.ROTATION_90 (Paisagem, 90 graus)
     * - Surface.ROTATION_180 (Retrato invertido, 180 graus)
     * - Surface.ROTATION_270 (Paisagem invertida, 270 graus)
     */
    public static void forceRotation(Context context, int rotation) {
        if (rotation < Surface.ROTATION_0 || rotation > Surface.ROTATION_270) {
            Slog.w(TAG, "Tentativa de rotacionar para um valor inválido: " + rotation);
            return;
        }

        Slog.d(TAG, "Forçando a rotação para " + rotation + " e travando.");
        
        // O segundo parâmetro é o ângulo para o qual travar.
        RotationPolicy.setRotationLockAtAngle(context, true, rotation);
    }

    /**
     *
     * @param context O contexto do sistema.
     */
    public static void restoreDefaultRotationPolicy(Context context) {
        Slog.d(TAG, "Restaurando a política de rotação padrão do usuário.");
        
        RotationPolicy.setRotationLock(context, false);
    }
}