package com.ecowater;

import androidx.test.core.app.ActivityScenario;
import androidx.test.espresso.core.internal.deps.guava.collect.Iterables;
import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.action.ViewActions.click;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import androidx.test.espresso.intent.Intents;
import static androidx.test.ext.truth.content.IntentSubject.assertThat;

import android.content.Intent;

@RunWith(AndroidJUnit4.class)
public class SensorActivityTest {
    @Rule
    public ActivityScenarioRule<MainActivity> activityRule = new ActivityScenarioRule(MainActivity.class);

    @Before
    public void intentsInit() {
        // initialize Espresso Intents capturing
        Intents.init();
    }

    @After
    public void intentsTeardown() {
        // release Espresso Intents capturing
        Intents.release();
    }

    @Test
    public void testShowSensorButton()
    {
        ActivityScenario<MainActivity> scenario = activityRule.getScenario();
        onView(withId(R.id.sensorsBtn)).perform(click());
        assertThat((Intent) Iterables.getOnlyElement(Intents.getIntents())).hasComponentClass(
                Sensors.class);
    }
}
